/*
 * drivers/input/touchscreen/tap2unlock.c
 *
 *
 * Copyright (c) 2013, Dennis Rassmann <showp1984@gmail.com>
 * Copyright (c) 2014, goutamniwas <goutamniwas@gmail.com>
 * Copyright (c) 2015, Swapnil Solanki <swapnil133609@gmail.com>
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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/input/tap2unlock.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <linux/input.h>
#include <linux/hrtimer.h>
#include <asm-generic/cputime.h>

#define WAKE_HOOKS_DEFINED

#ifndef WAKE_HOOKS_DEFINED
#ifndef CONFIG_HAS_EARLYSUSPEND
#include <linux/lcd_notify.h>
#else
#include <linux/earlysuspend.h>
#endif
#endif

/* uncomment since no touchscreen defines android touch, do that here */
//#define ANDROID_TOUCH_DECLARED

/* if Sweep2Wake is compiled it will already have taken care of this */
#ifdef CONFIG_TOUCHSCREEN_SWEEP2WAKE
#define ANDROID_TOUCH_DECLARED
#endif

/* Version, author, desc, etc */
#define DRIVER_AUTHOR "goutamniwas <goutamniwas@gmail.com>"
#define DRIVER_DESCRIPTION "tap2unlock for almost any device"
#define DRIVER_VERSION "3.0"
#define LOGTAG "[tap2unlock]: "

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESCRIPTION);
MODULE_VERSION(DRIVER_VERSION);
MODULE_LICENSE("GPLv2");

/* Tuneables */
#define t2u_DEBUG		0


#define t2u_PWRKEY_DUR		60
#define t2u_TIME		600 //gap(in ms) allowed between 'each' touch (for 4 letter pattern - 4x600 =2400 ms)
#define VERTICAL_SCREEN_MIDWAY  427 // Your device's vertical resolution / 2
#define HORIZONTAL_SCREEN_MIDWAY  240 // Your device's horizontal resolution / 2

/* Resources */
int t2u_switch = 0;
int static t2u_pattern[4] = {1,2,3,4};
bool t2u_scr_suspended = false, pass = false;
static cputime64_t tap_time_pre = 0;
static int touch_x = 0, touch_y = 0, touch_nr = 0;
static bool touch_x_called = false, touch_y_called = false, touch_cnt = false;
#ifndef WAKE_HOOKS_DEFINED
#ifndef CONFIG_HAS_EARLYSUSPEND
static struct notifier_block s2w_lcd_notif;
#endif
#endif
static struct input_dev * tap2unlock_pwrdev;
static DEFINE_MUTEX(pwrkeyworklock);
static struct workqueue_struct *t2u_input_wq;
static struct work_struct t2u_input_work;
bool t2u_allow = false,incall_active = false,touch_isactive = false,t2u_duplicate_allow = false;

/* Read cmdline for t2u */
static int __init read_t2u_cmdline(char *t2u)
{
	if (strcmp(t2u, "1") == 0) {
		pr_info("[cmdline_t2u]: tap2unlock enabled. | t2u='%s'\n", t2u);
		t2u_switch = 1;
	} else if (strcmp(t2u, "0") == 0) {
		pr_info("[cmdline_t2u]: tap2unlock disabled. | t2u='%s'\n", t2u);
		t2u_switch = 0;
	} else {
		pr_info("[cmdline_t2u]: No valid input found. Going with default: | t2u='%u'\n", t2u_switch);
	}
	return 1;
}
__setup("t2u=", read_t2u_cmdline);


/* PowerKey work func */
static void tap2unlock_presspwr(struct work_struct * tap2unlock_presspwr_work) {
	if (!mutex_trylock(&pwrkeyworklock))
                return;
	input_event(tap2unlock_pwrdev, EV_KEY, KEY_POWER, 1);
	input_event(tap2unlock_pwrdev, EV_SYN, 0, 0);
	msleep(t2u_PWRKEY_DUR);
	input_event(tap2unlock_pwrdev, EV_KEY, KEY_POWER, 0);
	input_event(tap2unlock_pwrdev, EV_SYN, 0, 0);
	msleep(t2u_PWRKEY_DUR);
        mutex_unlock(&pwrkeyworklock);
	return;
}
static DECLARE_WORK(tap2unlock_presspwr_work, tap2unlock_presspwr);

/* PowerKey trigger */
static void tap2unlock_pwrtrigger(void) {
	schedule_work(&tap2unlock_presspwr_work);
        return;
}

/* unsigned - calculate the region where the touch has occured */
static unsigned int calc_feather(int coord, int prev_coord) {


	if (prev_coord < VERTICAL_SCREEN_MIDWAY)
		if (coord < HORIZONTAL_SCREEN_MIDWAY)
			return 1;
		else
			return 2;
	else
		if (coord < HORIZONTAL_SCREEN_MIDWAY)
			return 3;
		else
			return 4;

}


/* tap2unlock main function */
static void detect_tap2unlock(int x, int y)
{
#if t2u_DEBUG
        pr_info(LOGTAG"x,y(%4d,%4d)",x, y);
#endif
	if ((t2u_switch > 0) && (touch_cnt)) {

		touch_cnt = false;
		//check if time gap between two touches is less than t2u_TIME
		if(touch_nr > 0 && (ktime_to_ms(ktime_get()) - tap_time_pre) > t2u_TIME)
			touch_nr = 0;

		if (touch_nr < 3) {

			if (calc_feather(x, y) == t2u_pattern[touch_nr]) {
				tap_time_pre = ktime_to_ms(ktime_get());
				touch_nr++;
			}
			else
				touch_nr = 0;

		} else  {//when touch_nr ==3 i.e on the 4 th knock , it wont be allowed any further than that

			 if (calc_feather(x, y) == t2u_pattern[3]) {
				t2u_allow = true;
				t2u_duplicate_allow = true;
				pr_info(LOGTAG"T2U : ON\n");
				touch_nr = 0;
				tap2unlock_pwrtrigger();
			}
			else
				touch_nr = 0;
		}

	}
}

static void t2u_input_callback(struct work_struct *unused) {

	detect_tap2unlock(touch_x, touch_y);

	return;
}

static void t2u_input_event(struct input_handle *handle, unsigned int type,
				unsigned int code, int value) {
#if t2u_DEBUG
	pr_info("tap2unlock: code: %s|%u, val: %i\n",
		((code==ABS_MT_POSITION_X) ? "X" :
		(code==ABS_MT_POSITION_Y) ? "Y" :
		((code==ABS_MT_TRACKING_ID)||
			(code==330)) ? "ID" : "undef"), code, value);
#endif
	if (!t2u_scr_suspended)
		return;

	if (code == ABS_MT_SLOT) {
		touch_nr = 0;
		return;
	}

	if ((code == ABS_MT_TRACKING_ID && value == -1) ||
		(code == 330 && value == 0)) {
		touch_cnt = true;
		return;
	}

	if (code == ABS_MT_POSITION_X) {
		touch_x = value;
		touch_x_called = true;
	}

	if (code == ABS_MT_POSITION_Y) {
		touch_y = value;
		touch_y_called = true;
	}

	if (touch_x_called || touch_y_called) {
		touch_x_called = false;
		touch_y_called = false;
		queue_work_on(0, t2u_input_wq, &t2u_input_work);
	}
}

static int input_dev_filter(struct input_dev *dev) {
	if (strstr(dev->name, "touch") ||
	    strstr(dev->name, "mtk-tpd")) {
		return 0;
	} else {
		return 1;
	}
}

static int t2u_input_connect(struct input_handler *handler,
				struct input_dev *dev, const struct input_device_id *id) {
	struct input_handle *handle;
	int error;

	if (input_dev_filter(dev))
		return -ENODEV;

	handle = kzalloc(sizeof(struct input_handle), GFP_KERNEL);
	if (!handle)
		return -ENOMEM;

	handle->dev = dev;
	handle->handler = handler;
	handle->name = "t2u";

	error = input_register_handle(handle);
	if (error)
		goto err2;

	error = input_open_device(handle);
	if (error)
		goto err1;

	return 0;
err1:
	input_unregister_handle(handle);
err2:
	kfree(handle);
	return error;
}

static void t2u_input_disconnect(struct input_handle *handle) {
	input_close_device(handle);
	input_unregister_handle(handle);
	kfree(handle);
}

static const struct input_device_id t2u_ids[] = {
	{ .driver_info = 1 },
	{ },
};

static struct input_handler t2u_input_handler = {
	.event		= t2u_input_event,
	.connect	= t2u_input_connect,
	.disconnect	= t2u_input_disconnect,
	.name		= "t2u_inputreq",
	.id_table	= t2u_ids,
};

#ifndef WAKE_HOOKS_DEFINED
#ifndef CONFIG_HAS_EARLYSUSPEND
static int lcd_notifier_callback(struct notifier_block *this,
				unsigned long event, void *data)
{
	switch (event) {
	case LCD_EVENT_ON_END:
		t2u_scr_suspended = false;
		break;
	case LCD_EVENT_OFF_END:
		t2u_scr_suspended = true;
		break;
	default:
		break;
	}

	return 0;
}
#else
static void t2u_early_suspend(struct early_suspend *h) {
	t2u_scr_suspended = true;
}

static void t2u_late_resume(struct early_suspend *h) {
	t2u_scr_suspended = false;
}

static struct early_suspend t2u_early_suspend_handler = {
	.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN,
	.suspend = t2u_early_suspend,
	.resume = t2u_late_resume,
};
#endif
#endif

/*
 * SYSFS stuff below here
 */
static ssize_t t2u_tap2unlock_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	size_t count = 0;

	count += sprintf(buf, "%d\n", t2u_switch);

	return count;
}

static ssize_t t2u_tap2unlock_dump(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	if (buf[0] >= '0' && buf[0] <= '2' && buf[1] == '\n')
                if (t2u_switch != buf[0] - '0')
		        t2u_switch = buf[0] - '0';

	return count;
}

static DEVICE_ATTR(tap2unlock, (S_IWUSR|S_IRUGO),
	t2u_tap2unlock_show, t2u_tap2unlock_dump);

static ssize_t t2u_pattern_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	size_t count = 0;

	count += sprintf(buf, "%d", pass);

	return count;
}

static ssize_t t2u_pattern_dump(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	if (t2u_pattern[0] == buf[0] - '0' && t2u_pattern[1] == buf[1] - '0' &&
				t2u_pattern[2] == buf[2] - '0' && t2u_pattern[3] == buf[3] - '0' )
	{
		pass = true;

		t2u_pattern[0] = buf[4] - '0';
		t2u_pattern[1] = buf[5] - '0';
		t2u_pattern[2] = buf[6] - '0';
		t2u_pattern[3] = buf[7] - '0';
	}
	else
		pass = false;

	return count;
}

static DEVICE_ATTR(tap2unlock_pattern, (S_IWUSR|S_IRUGO),
	t2u_pattern_show, t2u_pattern_dump);

static ssize_t t2u_version_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	size_t count = 0;

	count += sprintf(buf, "%s\n", DRIVER_VERSION);

	return count;
}

static ssize_t t2u_version_dump(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	return count;
}

static DEVICE_ATTR(tap2unlock_version, (S_IWUSR|S_IRUGO),
	t2u_version_show, t2u_version_dump);

static ssize_t t2u_allow_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	size_t count = 0;

	count += sprintf(buf, "%d\n", t2u_duplicate_allow);

	return count;
}

static ssize_t t2u_allow_dump(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	if (t2u_pattern[0] == buf[0] - '0' && t2u_pattern[1] == buf[1] - '0' && t2u_pattern[2] == buf[2] - '0' && t2u_pattern[3] == buf[3] - '0')
	{


		if(buf[4] == '1') {

				t2u_allow = true;
				incall_active = true;
				//touch_isactive = true;

		}
		else if (buf[4] == '0') {

				t2u_allow = false;
				incall_active = false;
				//touch_isactive = false;

		}
	}

	return count;
}

static DEVICE_ATTR(t2u_allow, (S_IWUSR|S_IRUGO),
	t2u_allow_show, t2u_allow_dump);

/*
 * INIT / EXIT stuff below here
 */
#ifdef ANDROID_TOUCH_DECLARED
extern struct kobject *android_touch_kobj;
#else
struct kobject *android_touch_kobj;
EXPORT_SYMBOL_GPL(android_touch_kobj);
#endif
static int __init tap2unlock_init(void)
{
	int rc = 0;

	tap2unlock_pwrdev = input_allocate_device();
	if (!tap2unlock_pwrdev) {
		pr_err("Can't allocate suspend autotest power button\n");
		goto err_alloc_dev;
	}

	input_set_capability(tap2unlock_pwrdev, EV_KEY, KEY_POWER);
	tap2unlock_pwrdev->name = "t2u_pwrkey";
	tap2unlock_pwrdev->phys = "t2u_pwrkey/input0";

	rc = input_register_device(tap2unlock_pwrdev);
	if (rc) {
		pr_err("%s: input_register_device err=%d\n", __func__, rc);
		goto err_input_dev;
	}

	t2u_input_wq = create_workqueue("t2uiwq");
	if (!t2u_input_wq) {
		pr_err("%s: Failed to create t2uiwq workqueue\n", __func__);
		return -EFAULT;
	}
	INIT_WORK(&t2u_input_work, t2u_input_callback);
	rc = input_register_handler(&t2u_input_handler);
	if (rc)
		pr_err("%s: Failed to register t2u_input_handler\n", __func__);

#ifndef WAKE_HOOKS_DEFINED
#ifndef CONFIG_HAS_EARLYSUSPEND
	t2u_lcd_notif.notifier_call = lcd_notifier_callback;
	if (lcd_register_client(&t2u_lcd_notif) != 0) {
		pr_err("%s: Failed to register lcd callback\n", __func__);
	}
#else
	register_early_suspend(&t2u_early_suspend_handler);
#endif
#endif

#ifndef ANDROID_TOUCH_DECLARED
	android_touch_kobj = kobject_create_and_add("tap2unlock", NULL) ;
	if (android_touch_kobj == NULL) {
		pr_warn("%s: android_touch_kobj create_and_add failed\n", __func__);
	}
#endif
	rc = sysfs_create_file(android_touch_kobj, &dev_attr_tap2unlock.attr);
	if (rc) {
		pr_warn("%s: sysfs_create_file failed for tap2unlock\n", __func__);
	}
	rc = sysfs_create_file(android_touch_kobj, &dev_attr_tap2unlock_pattern.attr);
	if (rc) {
		pr_warn("%s: sysfs_create_file failed for pattern\n", __func__);
	}
	rc = sysfs_create_file(android_touch_kobj, &dev_attr_tap2unlock_version.attr);
	if (rc) {
		pr_warn("%s: sysfs_create_file failed for tap2unlock_version\n", __func__);
	}
	rc = sysfs_create_file(android_touch_kobj, &dev_attr_t2u_allow.attr);
	if (rc) {
		pr_warn("%s: sysfs_create_file failed for t2u_allow\n", __func__);
	}


err_input_dev:
	input_free_device(tap2unlock_pwrdev);
err_alloc_dev:
	pr_info(LOGTAG"%s done\n", __func__);

	return 0;
}

static void __exit tap2unlock_exit(void)
{
#ifndef ANDROID_TOUCH_DECLARED
	kobject_del(android_touch_kobj);
#endif
#ifndef WAKE_HOOKS_DEFINED
#ifndef CONFIG_HAS_EARLYSUSPEND
	lcd_unregister_client(&t2u_lcd_notif);
#endif
#endif
	input_unregister_handler(&t2u_input_handler);
	destroy_workqueue(t2u_input_wq);
	input_unregister_device(tap2unlock_pwrdev);
	input_free_device(tap2unlock_pwrdev);
	return;
}

module_init(tap2unlock_init);
module_exit(tap2unlock_exit);

