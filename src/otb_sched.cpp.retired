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

#define _TASK_LTS_POINTER
#define _TASK_STATUS_REQUEST
#include "TaskScheduler.h"

#define OTB_SCHED_CPP
#include "otb_cpp.h"

extern "C" void ICACHE_FLASH_ATTR *otb_sched_get_task(int num)
{
  void *task = NULL;
  
  DEBUG("SCHED: otb_sched_get_task entry");

  if (num ==  OTB_SCHED_MQTT_TASK)
  {
    DEBUG("MQTT task");
    task = (void *)&task1;
  }
  else if (num == OTB_SCHED_OW_TASK)
  {
    DEBUG("OW task");
    task = (void *)&task2;
  }
  else
  {
    OTB_ASSERT(FALSE);
  }
  
  DEBUG("SCHED: otb_sched_get_task exit");
  
  return (task);
}

extern "C" void ICACHE_FLASH_ATTR otb_sched_repeat_task(void *vTask,
                                                         unsigned long aInterval,
                                                         long aIterations) 
{
  Task *task;
 
  DEBUG("SCHED: otb_sched_repeat_task entry");
 
  task = (Task *)vTask;
  task->set(aInterval, aIterations, &otb_sched_fake_callback);

  DEBUG("SCHED: otb_sched_repeat_task exit");

  return;
}

extern "C" void ICACHE_FLASH_ATTR *otb_sched_get_sr(int num)
{
  void *sr = NULL;

  DEBUG("SCHED: otb_sched_get_sr entry");
  
  if (num == OTB_SCHED_MQTT_TASK)
  {
    DEBUG("MQTT SR");
    sr = (void *)&sr1;
  }  
  else if (num == OTB_SCHED_OW_TASK)
  {
    DEBUG("OW SR ");
    sr = (void *)&sr2;
  }
  else
  {
    OTB_ASSERT(FALSE);
  }
  
  DEBUG("SCHED: otb_sched_get_sr exit");
  
  return sr;
}

extern "C" bool ICACHE_FLASH_ATTR otb_sched_system_os_post(uint8 priority,
                                                           os_signal_t signal,
                                                           os_param_t parameter,
                                                           os_task_t realCallback,
                                                           void *vTask,
                                                           void *vSr)
{
  // Ignore priority (set by Arduino) and signal (always 0)

  // Key function is to:
  //  - Create params that allow us to store off params request provided, plus right callback
  //  - Create a TaskScheduler Task
  //  - Add LTS Pointer to parameter
  //  - Add task to scheduler
  Task *task;
  task = (Task*)vTask;
  StatusRequest *sr;
  bool rc = FALSE;
  
  DEBUG("SCHED: otb_sched_system_os_post entry");

  // Set up OTB_SCHED_EVENT_PARAMS to be params that be passed back to fake_callback
  OTB_SCHED_EVENT_PARAMS *event_params;
  event_params = (OTB_SCHED_EVENT_PARAMS *)os_malloc(sizeof(OTB_SCHED_EVENT_PARAMS));

  // Normally one wouldn't assert on a failed memory allocation, if this fails we're
  // basically screwed so we need to reset, and asserting will do this
  OTB_ASSERT(event_params != NULL);

  event_params->callback = realCallback;
  event_params->callbackParams = parameter;

  // Create task and add LTS (long term storage) pointer to it
  task->setLtsPointer(event_params);
  if (vSr)
  {
    DEBUG("One off task");
    // One off task
    sr = (StatusRequest *)vSr;
    sr->setWaiting();
    task->waitFor(sr);
    sr->signalComplete();
    event_params->repeat = FALSE;
  }
  else
  {
    DEBUG("Repeating task");
    // Repeating task
    event_params->repeat = TRUE;
    task->enable();
  }
  
  rc = TRUE;

  DEBUG("SCHED: otb_sched_system_os_post entry");

  return rc;
}

void ICACHE_FLASH_ATTR otb_sched_fake_callback()
{
  // Get the params
  os_event_t event;
  OTB_SCHED_EVENT_PARAMS *event_params;
  
  DEBUG("SCHED: otb_sched_fake_callback entry");
  
  event_params = (OTB_SCHED_EVENT_PARAMS*)scheduler.currentLts();
  
  event.par = event_params->callbackParams;
  
  event_params->callback(&event);
  if (!event_params->repeat)
  {
    DEBUG("Non repeating task, free off event_params or we'll leak memory");
    os_free(event_params);
    event_params = NULL;
  }

  DEBUG("SCHED: otb_sched_fake_callback exit");

  return;
}

void ICACHE_FLASH_ATTR otb_sched_execute()
{
  // DEBUG("SCHED: otb_sched_execute entry");

  scheduler.execute();

  // DEBUG("SCHED: otb_sched_execute exit");

 return;
}
