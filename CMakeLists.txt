# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
set(mcuboot_CONFIG_BOOT_SIGNATURE_KEY_FILE \"${CMAKE_CURRENT_SOURCE_DIR}/priv.pem\")
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(adhd)

target_sources(app PRIVATE
  src/main.c
  src/battery.c
  src/haptics.c
)
