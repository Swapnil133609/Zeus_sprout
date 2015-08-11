/*
 * include/linux/input/wake_helpers.h
 *
 * Copyright (c) 2015, Vineeth Raj <contact.twn@openmailbox.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef _LINUX_WAKE_HELPERS_H
#define _LINUX_WAKE_HELPERS_H

#include <stdbool.h>

extern int var_is_earpiece_on;
extern int headset_plugged_in;
extern int var_is_headset_in_use;

extern bool s2w_scr_suspended;
extern bool dt2w_scr_suspended;

int is_headset_in_use(void);
int is_earpiece_on(void);

extern int dt2w_sent_play_pause;

#endif  /* _LINUX_WAKE_HELPERS_H */
