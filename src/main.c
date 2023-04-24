#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>
#include <string.h>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_sleep.h"
#include "esp_system.h"
#include "esp_wifi.h"

#include "esp_camera.h"

#include "freertos/event_groups.h"

#include "nvs.h"
#include "nvs_flash.h"

#include <esp_http_server.h>

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1
#define PART_BOUNDARY "123456789000000000000987654321"

static const char *TAG = "INIT";

static const char *_STREAM_CONTENT_TYPE =
    "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;

static httpd_handle_t server = NULL;
static EventGroupHandle_t wifi_event_group;

esp_err_t get_handler(httpd_req_t *req) {
  const char resp[] = "URI GET Response";
  httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
  return ESP_OK;
}

// static esp_err_t stream_handler(httpd_req_t *req) {
//   esp_err_t res = ESP_OK;
//   res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
//   if (res != ESP_OK) {
//     return res;
//   }

//   while (true) {
//     fb = esp_camera_fb_get();
//     if (!fb) {
//       Serial.println("Camera capture failed");
//       res = ESP_FAIL;
//     } else {
//       if (fb->width > 400) {
//         if (fb->format != PIXFORMAT_JPEG) {
//           bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
//           esp_camera_fb_return(fb);
//           fb = NULL;
//           if (!jpeg_converted) {
//             Serial.println("JPEG compression failed");
//             res = ESP_FAIL;
//           }
//         } else {
//           _jpg_buf_len = fb->len;
//           _jpg_buf = fb->buf;
//         }
//       }
//     }
//     if (res == ESP_OK) {
//       size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART,
//       _jpg_buf_len); res = httpd_resp_send_chunk(req, (const char *)part_buf,
//       hlen);
//     }
//     if (res == ESP_OK) {
//       res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
//     }
//     if (res == ESP_OK) {
//       res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY,
//                                   strlen(_STREAM_BOUNDARY));
//     }
//     if (fb) {
//       esp_camera_fb_return(fb);
//       fb = NULL;
//       _jpg_buf = NULL;
//     } else if (_jpg_buf) {
//       free(_jpg_buf);
//       _jpg_buf = NULL;
//     }
//     if (res != ESP_OK) {
//       break;
//     }
//   }
//   return res;
// }

httpd_uri_t uri_get = {
    .uri = "/", .method = HTTP_GET, .handler = get_handler, .user_ctx = NULL};

static void start_webserver(void) {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.lru_purge_enable = true;

  ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
  if (httpd_start(&server, &config) == ESP_OK) {
    ESP_LOGI(TAG, "Registering URI handlers");
    httpd_register_uri_handler(server, &uri_get);
  }
}

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data) {

  if (event_base == WIFI_EVENT) {
    switch (event_id) {
    case WIFI_EVENT_STA_START:
      esp_wifi_connect();
      ESP_LOGI(TAG, "Wifi started");
      break;
    case WIFI_EVENT_STA_DISCONNECTED:
      ESP_LOGW(TAG, "Wifi disconnected");
      xEventGroupSetBits(wifi_event_group, WIFI_FAIL_BIT);
      break;
    case WIFI_EVENT_STA_CONNECTED:
      ESP_LOGI(TAG, "Wifi connected");
      break;
    default:
      ESP_LOGI(TAG, "Wifi event unhandled");
      break;
    }
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
    xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
  } else {
    ESP_LOGW(TAG, "Unhandled event [%s: %ld]", event_base, event_id);
  }
}

static void init_system(void) {
  ESP_LOGI(TAG, "Starting system");

  esp_err_t ret = nvs_flash_init();
  if ((ret == ESP_ERR_NVS_NO_FREE_PAGES) ||
      (ret == ESP_ERR_NVS_NEW_VERSION_FOUND)) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);
  wifi_event_group = xEventGroupCreate();
}

static void init_wifi(const char *ssid, const char *ssid_pass,
                      const char *hostname) {
  ESP_LOGI(TAG, "Connecting wifi at %s", ssid);
  ESP_ERROR_CHECK(esp_netif_init());

  ESP_ERROR_CHECK(esp_event_loop_create_default());
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                             &event_handler, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                             &event_handler, NULL));

  esp_netif_t *netif = esp_netif_create_default_wifi_sta();
  ESP_ERROR_CHECK(esp_netif_set_hostname(netif, hostname));

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  wifi_config_t wifi_config = {};
  strcpy((char *)wifi_config.sta.ssid, ssid);
  strcpy((char *)wifi_config.sta.password, ssid_pass);

  ESP_LOGI(TAG, "Setting WiFi configuration SSID %s...", wifi_config.sta.ssid);
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());
}

void app_main() {
  init_system();
  init_wifi("OliveiraTravi_2.4", "PinhosHouse2016", "micro-monitor");

  xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdFALSE,
                      portMAX_DELAY);

  start_webserver();
}