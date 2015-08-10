
/*
* include/linux/input/tap2unlock.h
*
* Copyright (c) 2013, Dennis Rassmann <showp1984@gmail.com>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
* FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
* more details.
*
* You should have received a copy of the GNU General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/
#ifndef _LINUX_TAP2UNLOCK_H
#define _LINUX_TAP2UNLOCK_H

extern bool t2u_scr_suspended;
extern bool t2u_wake;
extern int t2u_switch;
extern bool t2u_allow,incall_active,touch_isactive,t2u_duplicate_allow;

#endif /* _LINUX_TAP2UNLOCK_H */
