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

// Variable with module scope to hold callback function registered by this module
os_task_t moduleCallback = 0;
extern void fake_callback();
extern void *getTask(int);
extern void *repeatTask(void *, unsigned long, long);
extern void *getSr(int);

// Replace ESP system functions with fakes to subvert and replace with Arduino Task Scheduler function
#define system_os_task(A, B, C, D) moduleCallback = A
#define system_os_post(A, B, C) fake_system_os_post(A, B, C, moduleCallback, getTask(1), getSr(1))

