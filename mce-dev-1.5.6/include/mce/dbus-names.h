/**
 * @file dbus-names.h
 * DBus Interface to the Mode Control Entity
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
#ifndef _DBUS_NAMES_H_
#define _DBUS_NAMES_H_

/** The MCE DBus service */
#define MCE_SERVICE			"com.nokia.mce"

/** The MCE DBus Request interface */
#define MCE_REQUEST_IF			"com.nokia.mce.request"
/** The MCE DBus Signal interface */
#define MCE_SIGNAL_IF			"com.nokia.mce.signal"
/** The MCE DBus Request path */
#define MCE_REQUEST_PATH		"/com/nokia/mce/request"
/** The MCE DBus Signal path */
#define MCE_SIGNAL_PATH			"/com/nokia/mce/signal"

/** The MCE DBus error interface; currently not used */
#define MCE_ERROR_FATAL			"com.nokia.mce.error.fatal"
/** The DBus interface for invalid arguments; currently not used */
#define MCE_ERROR_INVALID_ARGS		"org.freedesktop.DBus.Error.InvalidArgs"

/**
 * Request device mode
 *
 * @since v0.5.3
 * @return @c dbus_string_t with the device mode
 *         (see @ref mce/mode-names.h for valid device modes)
 */
#define MCE_DEVICE_MODE_GET		"get_device_mode"

/**
 * Request device lock mode
 *
 * @since v0.8.0
 * @return @c dbus_string_t with the device lock mode
 *         (see @ref mce/mode-names.h for valid lock modes)
 */
#define MCE_DEVLOCK_MODE_GET		"get_devicelock_mode"

/**
 * Request touchscreen/keypad lock mode
 *
 * @since v1.4.0
 * @return @c dbus_string_t with the touchscreen/keypad lock mode
 *         (see @ref mce/mode-names.h for valid lock modes)
 */
#define MCE_TKLOCK_MODE_GET		"get_tklock_mode"

/**
 * Request display status
 *
 * @since v1.5.21
 * @return @c dbus_string_t with the display state
 *         (see @ref mce/mode-names.h for valid display states)
 */
#define MCE_DISPLAY_STATUS_GET		"get_display_status"

/**
 * Request inactivity status
 *
 * @since v1.5.2
 * @return @c dbus_bool_t @c TRUE if the system is inactive,
 *                        @c FALSE if the system is active
 */
#define MCE_INACTIVITY_STATUS_GET	"get_inactivity_status"

/**
 * Request MCE version
 *
 * @since v1.1.6
 * @return @c dbus_string_t with the MCE version
 */
#define MCE_VERSION_GET			"get_version"

/**
 * Unblank display
 *
 * @since v0.5
 */
#define MCE_DISPLAY_ON_REQ		"req_display_state_on"

/**
 * Prevent display from blanking
 *
 * @since v0.5
 */
#define MCE_PREVENT_BLANK_REQ		"req_display_blanking_pause"

/**
 * Request device mode change
 *
 * @since v0.5
 * @param mode @c dbus_string_t with the new device mode
 *             (see @ref mce/mode-names.h for valid device modes)
 */
#define MCE_DEVICE_MODE_CHANGE_REQ	"req_device_mode_change"

/**
 * Request tklock mode change
 *
 * @since v1.4.0
 * @param mode @c dbus_string_t with the new touchscreen/keypad lock mode
 *             (see @ref mce/mode-names.h for valid lock modes)
 */
#define MCE_TKLOCK_MODE_CHANGE_REQ	"req_tklock_mode_change"

/**
 * Request powerkey event triggering
 *
 * @since v1.5.3
 * @param longpress @c dbus_bool_t with the type of powerkey event to
 *                  trigger; FALSE == short powerkey press,
 *                           TRUE == long powerkey press
 */
#define MCE_TRIGGER_POWERKEY_EVENT_REQ	"req_trigger_powerkey_event"

/**
 * Request powerup
 *
 * @since v0.5
 */
#define MCE_POWERUP_REQ			"req_powerup"

/**
 * Request reboot
 *
 * @since v0.5.5
 */
#define MCE_REBOOT_REQ			"req_reboot"

/**
 * Request shutdown
 *
 * @since v0.5
 */
#define MCE_SHUTDOWN_REQ		"req_shutdown"

/**
 * Validate device lock code
 *
 * @since v0.9.10
 * @param code @c dbus_string_t with the encrypted password from @c crypt(3)
 * @param salt @c dbus_string_t with the salt used with @c crypt(3)
 * @return @c dbus_bool_t @c TRUE if the lock code is correct,
 *                        @c FALSE if the lock code is incorrect
 */
#define MCE_DEVLOCK_VALIDATE_CODE_REQ	"validate_devicelock_code"

/**
 * Change device lock code
 *
 * @since v1.5.0
 * @param code @c dbus_string_t with the old encrypted password from @c crypt(3)
 * @param salt @c dbus_string_t with the old salt used with @c crypt(3)
 * @param newcode @c dbus_string_t with the new password in plaintext
 * @return @c dbus_bool_t @c TRUE if the lock code was modified,
 *                        @c FALSE if the lock code was not modified
 */
#define MCE_DEVLOCK_CHANGE_CODE_REQ	"change_devicelock_code"

/**
 * Set alarm mode
 *
 * @since v0.9.10
 * @param mode @c dbus_string_t with the new alarm mode
 *             (see @ref mce/mode-names.h for valid alarm modes)
 */
#define MCE_ALARM_MODE_CHANGE_REQ	"set_alarm_mode"

/**
 * Notify everyone that the system is about to shut down
 *
 * @since v0.4.1
 */
#define MCE_SHUTDOWN_SIG		"shutdown_ind"

/**
 * Notify everyone that the system is about to shut down
 * due to thermal constraints
 *
 * @since v1.5.4
 */
#define MCE_THERMAL_SHUTDOWN_SIG	"thermal_shutdown_ind"

/**
 * Signal that indicates that the device lock mode has changed
 *
 * @since v0.8.0
 * @return @c dbus_string_t with the new device lock mode
 *         (see @ref mce/mode-names.h for valid lock modes)
 */
#define MCE_DEVLOCK_MODE_SIG		"devicelock_mode_ind"

/**
 * Signal that indicates that the touchscreen/keypad lock mode has changed
 *
 * @since v1.4.0
 * @return @c dbus_string_t with the new touchscreen/keypad lock mode
 *         (see @ref mce/mode-names.h for valid lock modes)
 */
#define MCE_TKLOCK_MODE_SIG		"tklock_mode_ind"

/**
 * Notify everyone to save unsaved data
 *
 * @since v0.3
 */
#define MCE_DATA_SAVE_SIG		"save_unsaved_data_ind"

/**
 * Notify everyone that the display is on/dimmed/off
 *
 * @since v1.5.21
 * @return @c dbus_string_t with the display state
 *         (see @ref mce/mode-names.h for valid display states)
 */
#define MCE_DISPLAY_SIG			"display_status_ind"

/**
 * Notify everyone that the system is active/inactive
 *
 * @since v0.9.3
 * @return @c dbus_bool_t @c TRUE if the system is inactive,
 *                        @c FALSE if the system is active
 */
#define MCE_INACTIVITY_SIG		"system_inactivity_ind"

/**
 * Notify everyone that the device mode has changed
 *
 * @since v0.5
 * @return @c dbus_string_t with the new device mode
 *         (see @ref mce/mode-names.h for valid device modes)
 */
#define MCE_DEVICE_MODE_SIG		"sig_device_mode_ind"

/**
 * Notify everyone that the home button was pressed (short press)
 *
 * @since v0.3
 */
#define MCE_HOME_KEY_SIG		"sig_home_key_pressed_ind"

/**
 * Notify everyone that the home button was pressed (long press)
 *
 * @since v0.3
 */
#define MCE_HOME_KEY_LONG_SIG		"sig_home_key_pressed_long_ind"

/**
 * Activates a pre-defined LED pattern
 * Non-existing patterns are ignored
 *
 * @since v1.5.0
 * @param pattern @c dbus_string_t with the pattern name
 *                (see @ref /etc/mce/mce.ini for valid pattern names)
 */
#define MCE_ACTIVATE_LED_PATTERN	"req_led_pattern_activate"

/**
 * Deactivates a pre-defined LED pattern
 * Non-existing patterns are ignored
 *
 * @since v1.5.0
 * @param pattern @c dbus_string_t with the pattern name
 *                (see @ref /etc/mce/mce.ini for valid pattern names)
 */
#define MCE_DEACTIVATE_LED_PATTERN	"req_led_pattern_deactivate"

/**
 * Enable LED; this does not affect the LED pattern stack
 * Note: The GConf setting for LED flashing overrides this value
 *       If the pattern stack does not contain any active patterns,
 *       the LED logic will still remain in enabled mode,
 *       but will not display any patterns until a pattern is activated
 *
 * @since v1.5.0
 */
#define MCE_ENABLE_LED			"req_led_enable"

/**
 * Disable LED; this does not affect the LED pattern stack
 *
 * @since v1.5.0
 */
#define MCE_DISABLE_LED			"req_led_disable"

#endif /* _DBUS_NAMES_H_ */
