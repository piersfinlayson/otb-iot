/*
 * OTB-IOT - Out of The Box Internet Of Things
 *
 * Copyright (C) 2016 Piers Finlayson
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version. 
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of  MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for 
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#if 0
// Old inefficient inline (space wise) version
#define LOG(ERROR, FORMAT, ...)                                                \
  snprintf(otb_log_s, OTB_MAIN_MAX_LOG_LENGTH, (char *)FORMAT, ##__VA_ARGS__); \
  otb_util_log_fn(otb_log_s);                                                  \
  if (ERROR && (otb_mqtt_client.connState == MQTT_DATA))                       \
  {                                                                            \
    otb_util_log_error_via_mqtt(otb_log_s);                                    \
  }
#else
#define LOG(ERROR, FORMAT, ...)  otb_util_log(ERROR,                   \
                                              otb_log_s,               \
                                              OTB_MAIN_MAX_LOG_LENGTH, \
                                              (char *)FORMAT,          \
                                              ##__VA_ARGS__)
#endif

#ifndef OTB_RBOOT_BOOTLOADER

#define INFO(...)  LOG(FALSE, __VA_ARGS__)
#define WARN(...)  LOG(FALSE, __VA_ARGS__)
#define ERROR(...)  LOG(TRUE, __VA_ARGS__)

#ifndef ESPUT
#ifdef OTB_DEBUG
  #define DEBUG(...)  LOG(FALSE, __VA_ARGS__);
#else // OTB_DEBUG
  #define DEBUG(...)
#endif // OTB_DEBUG
#else // ESPUT
#define DEBUG(...)  if (esput_debug) { esput_printf("DEBUG LOG", __VA_ARGS__); esput_printf("", "\n");}
#endif

#ifndef ESPUT
// #X passes a stringified version of X, which is used for logging purposes
#define OTB_ASSERT(X) otb_util_assert(X, (char *)#X)
#else
#define OTB_ASSERT(X)                                                     \
{                                                                         \
  esput_printf("ASSERTION", "%s\n", #X);                                  \
  esput_printf("         ", "%s\n", (X) ? "passed" : "failed");           \
  if (!(X))                                                               \
  {                                                                       \
    esput_assertion_failed = TRUE;                                        \
  }                                                                       \
}

#endif // ESPUT

#else // OTB_RBOOT_BOOTLOADER

// Definitely don't want DEBUG (would make bootloader very large.  Probably do want other
// logs - but we need to call ets_printf directly, and add in CRLF.
#define INFO(FORMAT, ...)  ets_printf(FORMAT "\r\n", ##__VA_ARGS__)
#define WARN(FORMAT, ...)  ets_printf(FORMAT "\r\n", ##__VA_ARGS__)
#define ERROR(FORMAT, ...)  ets_printf(FORMAT "\r\n", ##__VA_ARGS__)
#define DEBUG(FORMAT, ...)  

#undef OTB_ASSERT
#define OTB_ASSERT(X)                                                         \
{                                                                             \
  if (!(X))                                                                   \
  {                                                                           \
    ets_printf("BOOT: Assertion failed!: %s\r\n",                             \
               #X);                                                           \
    ets_printf("                   File: %s\r\n",                             \
               __FILE__);                                                     \
    ets_printf("                   Line: %d\r\n",                             \
               __LINE__);                                                     \
    ets_printf("BOOT: Continuing ...\r\n");                                   \
  }                                                                           \
}

#endif // OTB_RBOOT_BOOTLOADER

#ifndef ESPUT
#define os_vsnprintf(A, B, ...)  ets_vsnprintf(A, B, __VA_ARGS__)
#define os_snprintf ets_snprintf
#else
#define os_vsnprintf(A, B, ...)  vsnprintf(A, B, __VA_ARGS__)
#define os_snprintf snprintf
#endif // ESPUT

// Macro to align stuff to 4 byte boundary.  Useful for reading and writing
// to flash
#define ALIGN4 __attribute__((aligned(4)))

#define OTB_MQTT_LOCATION_1  otb_conf->loc.loc1
#define OTB_MQTT_LOCATION_2  otb_conf->loc.loc2
#define OTB_MQTT_LOCATION_3  otb_conf->loc.loc3

#define OTB_MAX(A, B)  ((A > B) ? A : B)
#define OTB_MIN(A, B)  ((A < B) ? A : B)


