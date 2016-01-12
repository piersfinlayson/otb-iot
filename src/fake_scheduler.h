/*
 *
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
 *
 */

#define _TASK_LTS_POINTER
#define _TASK_STATUS_REQUEST
#include "TaskScheduler.h"
#include "osapi.h"
#include "os_type.h"
#include "mem.h"

extern "C"
{
  // Variable with module scope to hold callback function registered by this module
  //os_task_t *moduleCallback = 0;
  Scheduler scheduler;
  StatusRequest sr1;
  StatusRequest sr2;
  

  // Replace ESP system functions with fakes to subvert and replace with Arduino Task Scheduler function
  //#define system_os_task(A, B, C, D) moduleCallback = A
  //#define system_os_post(A, B, C) fake_system_os_post(A, B, C, moduleCallback)
  
  void fake_callback();
  Task task1 = Task(&fake_callback, &scheduler);
  Task task2 = Task(&fake_callback, &scheduler);

   void *getTask(int num)
   {
      if (num == 1)
        return (void *)&task1;
      else
        return (void *)&task2;
   }

   void *repeatTask(void *vTask, unsigned long aInterval, long aIterations) 
   {
     Task *task;
     task = (Task *)vTask;
     task->set(aInterval, aIterations, &fake_callback);
   }

   void *getSr(int num)
   {
      if (num == 1)
        return (void *)&sr1;
      else
        return (void *)&sr2;
   }


  // struct to wrap params provided by caller, and pass to TaskScheduler
  typedef struct eventParams
  {
    void (*callback)(os_event_t*);
    os_param_t callbackParams;
    bool repeat;
  
  } eventParams;

extern "C" void reset();
  bool ICACHE_FLASH_ATTR fake_system_os_post(uint8 priority, os_signal_t signal, os_param_t parameter, os_task_t realCallback, void *vTask, void *vSr)
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

    // Set up eventParams to be params that be passed back to fake_callback
    eventParams *event_params;
    event_params = (eventParams *)os_malloc(sizeof(eventParams));
    if (!event_params)
    {
      LOG("malloc failure in fake_system_os_post");
      reset();
    }
    event_params->callback = realCallback;
    event_params->callbackParams = parameter;
  
    //§LOG("Create task: event_params %p, callback %p params %p", event_params, realCallback, parameter);
    // Create task and add LTS pointer to it
    task->setLtsPointer(event_params);
    if (vSr)
    {
      // One off task
      sr = (StatusRequest *)vSr;
      sr->setWaiting();
      task->waitFor(sr);
      sr->signalComplete();
      event_params->repeat = FALSE;
    }
    else
    {
      // Repeating task
      event_params->repeat = TRUE;
      task->enable();
    }
    //LOG("           : fake_callback %p, scheduler %p, task %p", fake_callback, &scheduler, &task);

    // Enable it (this means it can be scheduled)
    //task.enable();
    //LOG("           : ... created");

    return 1;
  }

  void ICACHE_FLASH_ATTR fake_callback()
  {
    // Get the params
    os_event_t event;
    eventParams *event_params = (eventParams*)scheduler.currentLts();
    
    // Fake up what the callback expects to see
    event.par = event_params->callbackParams;
    
    //LOG("Callback: %p, event_params %p, params %p", event_params->callback, event_params, event.par);
    //delay(2000);
    // Call the real callback
    event_params->callback(&event);
    if (!event_params->repeat)
    {
      os_free(event_params);
      event_params = NULL;
    }
  }
  
}


