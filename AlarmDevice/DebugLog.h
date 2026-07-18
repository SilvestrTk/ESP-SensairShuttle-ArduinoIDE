/*
 * DebugLog.h — tagged, leveled Serial logging for the Alarm Device.
 *
 * Usage:  LOGI("NET", "connected, ip %s", ip.c_str());
 * Output: [   12345][I][NET ] connected, ip 192.168.1.7
 *
 * Level is set by APP_DEBUG_LEVEL in AppConfig.h; disabled statements
 * compile to nothing.
 */
#ifndef DEBUG_LOG_H
#define DEBUG_LOG_H

#include <Arduino.h>
#include "AppConfig.h"

#define LOG_PRINT(lvl, tag, fmt, ...) \
  Serial.printf("[%8lu][" lvl "][%-4s] " fmt "\r\n", (unsigned long)millis(), tag, ##__VA_ARGS__)

#if APP_DEBUG_LEVEL >= 1
#define LOGE(tag, fmt, ...) LOG_PRINT("E", tag, fmt, ##__VA_ARGS__)
#else
#define LOGE(tag, fmt, ...)
#endif

#if APP_DEBUG_LEVEL >= 2
#define LOGW(tag, fmt, ...) LOG_PRINT("W", tag, fmt, ##__VA_ARGS__)
#else
#define LOGW(tag, fmt, ...)
#endif

#if APP_DEBUG_LEVEL >= 3
#define LOGI(tag, fmt, ...) LOG_PRINT("I", tag, fmt, ##__VA_ARGS__)
#else
#define LOGI(tag, fmt, ...)
#endif

#if APP_DEBUG_LEVEL >= 4
#define LOGV(tag, fmt, ...) LOG_PRINT("V", tag, fmt, ##__VA_ARGS__)
#else
#define LOGV(tag, fmt, ...)
#endif

#endif /* DEBUG_LOG_H */
