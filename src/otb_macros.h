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

#define LOG(FORMAT, ...)                                                         \
    snprintf(otb_log_s, OTB_MAIN_MAX_LOG_LENGTH, (char *)FORMAT, ##__VA_ARGS__); \
    otb_main_log_fn(otb_log_s)    

#if 0
#define LOG(FORMAT, ...)  otb_util_log(otb_log_s,               \
                                       OTB_MAIN_MAX_LOG_LENGTH, \
                                       (char *)FORMAT,          \
                                       ##__VA_ARGS__)
#endif
#define INFO LOG
#define WARN LOG
#define ERROR LOG

#ifdef OTB_DEBUG
  #define DEBUG LOG
#else
  #define DEBUG(...)
#endif

// #X passes a stringified version of X, which is used for logging purposes
#define OTB_ASSERT(X) otb_util_assert(X, (char *)#X)
