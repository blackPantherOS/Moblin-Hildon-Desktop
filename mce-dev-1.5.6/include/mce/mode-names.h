/**
 * @file mode-names.h
 * Defines for names of various modes and submodes for Mode Control Entity
 * <p>
 * This file is part of mce-dev
 * <p>
 * Copyright (C) 2004-2006 Nokia Corporation.
 * <p>
 * @author David Weinehall <david.weinehall@nokia.com>
 *
 * These headers are free software; you can redistribute them
 * and/or modify them under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * These headers are distributed in the hope that they will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this software; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */
#ifndef _MODE_NAMES_H_
#define _MODE_NAMES_H_

/** Normal device mode */
#define MCE_NORMAL_MODE			"normal"	/* normal mode */
/** Offline device mode; RF's disabled */
#define MCE_FLIGHT_MODE			"flight"	/* flight mode */
/** Offline device mode; RF's disabled; alias for flight mode */
#define MCE_OFFLINE_MODE		"offline"	/* offline mode;
							 * unsupported!
							 */
/**
 * Variant of Normal mode customised for VoIP use;
 * this mode is not visible when the mode is queried
 */
#define MCE_VOIP_MODE			"voip"		/* voip mode;
							 * transparent
							 */
/** Invalid device mode; this should NEVER occur! */
#define MCE_INVALID_MODE		"invalid"	/* should *never*
							 * occur!
							 */

/** Device locked */
#define MCE_DEVICE_LOCKED		"locked"	/* device locked */
/** Device unlocked */
#define MCE_DEVICE_UNLOCKED		"unlocked"	/* device unlocked */

/** Touchscreen/Keypad locked */
#define MCE_TK_LOCKED			"locked"	/* locked */
/** Touchscreen/Keypad silently locked */
#define MCE_TK_SILENT_LOCKED		"silent-locked"	/* silently locked */
/** Touchscreen/Keypad locked with fadeout */
#define MCE_TK_LOCKED_DIM		"locked-dim"	/* locked
							 * with fadeout
							 */
/** Touchscreen/Keypad silently locked with fadeout */
#define MCE_TK_SILENT_LOCKED_DIM	"silent-locked-dim"	/* silently
								 * locked
								 * with fadeout
								 */
/** Touchscreen/Keypad unlocked */
#define MCE_TK_UNLOCKED			"unlocked"	/* unlocked */

/** Alarm UI is visible */
#define MCE_ALARM_VISIBLE		"visible"	/* alarm visible */
/** Alarm UI has been snoozed */
#define MCE_ALARM_SNOOZED		"snoozed"	/* alarm snoozed */
/** Alarm UI is not visible */
#define MCE_ALARM_OFF			"off"		/* alarm off */

/** Display state name for display on */
#define MCE_DISPLAY_ON_STRING		"on"		/* display on */
/** Display state name for display dim */
#define MCE_DISPLAY_DIM_STRING		"dimmed"	/* display dimmed */
/** Display state name for display off */
#define MCE_DISPLAY_OFF_STRING		"off"		/* display off */

#endif /* _MODE_NAMES_H_ */
