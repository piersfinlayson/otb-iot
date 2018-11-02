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
 
#ifndef OTB_MACROS_H_INCLUDED
#define OTB_MACROS_H_INCLUDED

// Some weird magic to allow us to create unique (per module) variable names
#define MUNGE2(X, Y)  X##Y
#define MUNGE1(X, Y)  MUNGE2(X, Y)
#define UNIQUE(B)     MUNGE1(B, __LINE__)

// Some weird magic to turn a number #define into a string
#define STRINGIFY2(X) #X
#define STRINGIFY(X)  STRINGIFY2(X)

// Macro to align stuff to 4 byte boundary.  Useful for reading and writing
// to flash.
#define ALIGN4 __attribute__((aligned(4)))

// If you hit a compile time error before conf has evaluated to 0 then you have hit a
// compile time assert (the compiler will fail to compile the macro if cond is 0
// Figure out how the assert has failed and fix it.  Intended to be used to check that
// logs aren't defined to be too big for the RAM buffer they'll be copied into when
// logged.  May look inefficient but the compiler should optimise this out.
#define OTB_COMPILE_ASSERT(cond) switch(0){case 0: case cond:;}

// The below should really be in otb_util.h but can't be there cos otb_macros.h gets
// included before otb_util.h
#define OTB_UTIL_LOG_FLASH_BUFFER_LEN 128
extern char ALIGN4 otb_util_log_flash_buffer[OTB_UTIL_LOG_FLASH_BUFFER_LEN];

// Used when you want to log a variable format rather than a fixed one which can be 
// allocated and compile time and stored on flash (in which case use LOG).
#define LOG_VAR(ERROR, FORMAT, ...) \
                                  otb_util_log(ERROR,                          \
                                              otb_log_s,                       \
                                              OTB_MAIN_MAX_LOG_LENGTH,         \
                                              FORMAT,                          \
                                              ##__VA_ARGS__)

// Complicated bit of code to avoid storing logs in RAM:
// - Generate a unique (to the scope of this module) variable name to hold the log text,
//   which will be stored in flash
// - Check the log isn't longer than the maximum size of the buffer we'll copy that string
//   out into later at runtime when the log is made - minus 4 bytes (will become clear
//   shortly.
// - Figure out the string size in 4 byte chunks, as we need to access the flash in these
//   chunks.  However, the length is either going to be 0, 1, 2, or 3 bytes more than a
//   a 4 byte length.  So we add an extra 4 bytes to be sure - this will copy 4, 3, 2 or 1
//   more bytes than necessary, but this isn't a problem as we've saved a buffer 4 bytes
//   bigger than the max log length (and will just copy some random stuff off the flash
//   to after the log is null terminated).
// - Do the copy, a 4 byte word at a time.
// - Log it!
#define LOG(ERROR, FORMAT, ...)                                                          \
{                                                                                        \
  char ALIGN4 *log_tmp_str;                                                              \
  size_t log_str_size;                                                                   \
  int log_ii;                                                                            \
  static const char ALIGN4 ICACHE_RODATA_ATTR UNIQUE(log)[] = FORMAT;                    \
  log_str_size = sizeof(UNIQUE(log))/sizeof(UNIQUE(log)[0]);                             \
  OTB_COMPILE_ASSERT(sizeof(UNIQUE(log)) <= (OTB_UTIL_LOG_FLASH_BUFFER_LEN));            \
  for (log_ii = 0; log_ii < ((log_str_size/4)+1); log_ii++)                              \
  {                                                                                      \
    *(((uint32_t*)(&otb_util_log_flash_buffer))+log_ii) =                                \
                                                 *(((uint32_t*)(&(UNIQUE(log))))+log_ii);\
  }                                                                                      \
  LOG_VAR(ERROR, otb_util_log_flash_buffer, ##__VA_ARGS__);                              \
}
                                              
#ifndef OTB_RBOOT_BOOTLOADER

#define INFO(...)   LOG(FALSE, __VA_ARGS__)
#define WARN(...)   LOG(FALSE, __VA_ARGS__)
#define ERROR(...)  LOG(TRUE, __VA_ARGS__)

#define INFO_VAR(...)   LOG_VAR(FALSE, __VA_ARGS__)
#define WARN_VAR(...)   LOG_VAR(FALSE, __VA_ARGS__)
#define ERROR_VAR(...)  LOG_VAR(TRUE, __VA_ARGS__)

#ifndef ESPUT
#ifdef OTB_DEBUG
  #define DEBUG(...)      LOG(FALSE, __VA_ARGS__)
  #define DEBUG_VAR(...)  LOG_VAR(FALSE, __VA_ARGS__)
#else // OTB_DEBUG
  #define DEBUG(...)
  #define DEBUG_VAR(...)
#endif // OTB_DEBUG
#else // ESPUT
#define DEBUG(...)  if (esput_debug) { esput_printf("DEBUG LOG", __VA_ARGS__); esput_printf("", "\n");}
#define DEBUG_VAR(...)  DEBUG(##__VA_ARGS__)
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

#define INFO_VAR(FORMAT, ...)  INFO(FORMAT, ##__VA_ARGS__)
#define WARN_VAR(FORMAT, ...)  WARN(FORMAT, ##__VA_ARGS__)
#define ERROR_VAR(FORMAT, ...)  ERROR(FORMAT, ##__VA_ARGS__)
#define DEBUG_VAR(FORMAT, ...)  DEBUG(FORMAT, ##__VA_ARGS__)

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
#ifndef os_snprintf
#define os_snprintf ets_snprintf
#endif
#else
#define os_vsnprintf(A, B, ...)  vsnprintf(A, B, __VA_ARGS__)
#define os_snprintf snprintf
#endif // ESPUT

#define OTB_MQTT_LOCATION_1  otb_conf->loc.loc1
#define OTB_MQTT_LOCATION_2  otb_conf->loc.loc2
#define OTB_MQTT_LOCATION_3  otb_conf->loc.loc3

#define OTB_MAX(A, B)  ((A > B) ? A : B)
#define OTB_MIN(A, B)  ((A < B) ? A : B)

#endif // OTB_MACROS_H_INCLUDED
