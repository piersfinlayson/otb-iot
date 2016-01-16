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


// struct to wrap params provided by caller, and pass to TaskScheduler
typedef struct eventParams
{
  void (*callback)(os_event_t*);
  os_param_t callbackParams;
  bool repeat;
} OTB_SCHED_EVENT_PARAMS;

#ifdef OTB_SCHED_CPP

// Variables with module scope to hold callback function registered by this module
Scheduler scheduler;
StatusRequest sr1;
StatusRequest sr2;

void otb_sched_fake_callback();

Task task1 = Task(&otb_sched_fake_callback, &scheduler);
Task task2 = Task(&otb_sched_fake_callback, &scheduler);
  
extern "C" void *otb_sched_get_task(int num);
extern "C" void otb_sched_repeat_task(void *vTask,
                                      unsigned long aInterval,
                                      long aIterations);
extern "C" void *otb_sched_get_sr(int num);
extern "C" bool ICACHE_FLASH_ATTR otb_sched_system_os_post(uint8 priority,
                                                           os_signal_t signal,
                                                           os_param_t parameter,
                                                           os_task_t realCallback,
                                                           void *vTask,
                                                           void *vSr);
extern "C" void otb_sched_execute();
#else

#ifdef OTB_MQTT_C
#define system_os_task(A, B, C, D)  otb_mqtt_module_callback = A
#define system_os_post(A, B, C)  otb_sched_system_os_post(A,                            \
                                                          B,                            \
                                                          C,                            \
                                                          otb_mqtt_module_callback,     \
                                                          getTask(OTB_SCHED_MQTT_TASK), \
                                                          getSr(OTB_SCHED_MQTT_TASK))
#endif

extern void *otb_sched_get_task(int num);
extern void *otb_sched_repeat_task(void *vTask,
                                   unsigned long aInterval,
                                   long aIterations);
extern void *otb_sched_get_sr(int num);
extern bool ICACHE_FLASH_ATTR otb_sched_system_os_post(uint8 priority,
                                                       os_signal_t signal,
                                                       os_param_t parameter,
                                                       os_task_t realCallback,
                                                       void *vTask,
                                                       void *vSr);
extern void otb_sched_execute();
#endif