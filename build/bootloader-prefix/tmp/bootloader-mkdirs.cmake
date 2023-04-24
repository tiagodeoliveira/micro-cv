# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/Users/tiagode/esp/esp-idf/components/bootloader/subproject"
  "/Users/tiagode/src/github.com/tiagodeoliveira/micro-monitoring/build/bootloader"
  "/Users/tiagode/src/github.com/tiagodeoliveira/micro-monitoring/build/bootloader-prefix"
  "/Users/tiagode/src/github.com/tiagodeoliveira/micro-monitoring/build/bootloader-prefix/tmp"
  "/Users/tiagode/src/github.com/tiagodeoliveira/micro-monitoring/build/bootloader-prefix/src/bootloader-stamp"
  "/Users/tiagode/src/github.com/tiagodeoliveira/micro-monitoring/build/bootloader-prefix/src"
  "/Users/tiagode/src/github.com/tiagodeoliveira/micro-monitoring/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/Users/tiagode/src/github.com/tiagodeoliveira/micro-monitoring/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/Users/tiagode/src/github.com/tiagodeoliveira/micro-monitoring/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
