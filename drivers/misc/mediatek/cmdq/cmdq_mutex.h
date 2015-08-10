/*
* Copyright (C) 2011-2014 MediaTek Inc.
*
* This program is free software: you can redistribute it and/or modify it under the terms of the
* GNU General Public License version 2 as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
* without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See the GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along with this program.
* If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __CMDQ_MUTEX_H__
#define __CMDQ_MUTEX_H__

#include <linux/mutex.h>
#include <ddp_hal.h>

#ifdef __cplusplus
extern "C" {
#endif

int32_t cmdqMutexInitialize(void);

int32_t cmdqMutexAcquire(void);

int32_t cmdqMutexRelease(int32_t mutex);

void cmdqMutexDeInitialize(void);

bool cmdqMDPMutexInUse(int index);

pid_t cmdqMDPMutexOwnerPid(int index);


#ifdef __cplusplus
}
#endif

#endif  // __CMDQ_MUTEX_H__
