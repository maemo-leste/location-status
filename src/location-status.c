/*
 * Copyright (c) 2020-2021 Ivan J. <parazyd@dyne.org>
 * Copyright (c) 2015      Ivaylo Dimitrov <ivo.g.dimitrov.75@gmail.com>
 *
 * This file is part of location-status
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include <config.h>

#include <dbus/dbus-glib-lowlevel.h>
#include <hildon/hildon.h>
#include <libhildondesktop/libhildondesktop.h>
#include <libosso.h>
#include <location/location-gps-device.h>

/* Use this for debugging */
#include <syslog.h>
#define status_debug(...) syslog(1, __VA_ARGS__)

typedef struct _LocationStatusMenuItem        LocationStatusMenuItem;
typedef struct _LocationStatusMenuItemClass   LocationStatusMenuItemClass;
typedef struct _LocationStatusMenuItemPrivate LocationStatusMenuItemPrivate;

struct _LocationStatusMenuItem
{
	HDStatusMenuItem parent;
	LocationStatusMenuItemPrivate *priv;
};

struct _LocationStatusMenuItemClass
{
	HDStatusMenuItemClass parent;
};

typedef enum {
	STATUS_ICON_NOT_CONNECTED,
	STATUS_ICON_SEARCHING,
	STATUS_ICON_FOUND
} CurStatusIcon;

struct _LocationStatusMenuItemPrivate
{
	osso_context_t *osso;
	DBusConnection *dbus;
	GdkPixbuf *pix18_gps_searching;
	GdkPixbuf *pix18_gps_location;
	GdkPixbuf *pix18_gps_not_connected;
	GdkPixbuf *pix48_gps_location;
	GdkPixbuf *pix48_gps_not_connected;
	int locationdaemon_running;
	LocationGPSDeviceMode prev_mode;
	LocationGPSDeviceMode curr_mode;
	CurStatusIcon current_status_icon;
	GtkWidget *status_button;
};

GType location_status_menu_item_get_type(void);

HD_DEFINE_PLUGIN_MODULE_WITH_PRIVATE(LocationStatusMenuItem,
	location_status_menu_item, HD_TYPE_STATUS_MENU_ITEM);

#define GET_PRIVATE(x) location_status_menu_item_get_instance_private(x)

static int execute_cp_plugin(GtkWidget *btn, LocationStatusMenuItem *obj)
{
	LocationStatusMenuItemPrivate *p = GET_PRIVATE(obj);
	GtkWidget *toplevel = gtk_widget_get_toplevel(GTK_WIDGET(obj));
	gtk_widget_hide(toplevel);

	if (osso_cp_plugin_execute(
			p->osso, "liblocation_applet.so", obj, TRUE) == OSSO_ERROR)
		hildon_banner_show_information(NULL, NULL,
			"Failed to show location settings");

	return 0;
}

static void set_status_icon(gpointer obj, GdkPixbuf *pixbuf)
{
	hd_status_plugin_item_set_status_area_icon(
		HD_STATUS_PLUGIN_ITEM(obj), pixbuf);
}

static void set_button_icon(gpointer obj, GdkPixbuf *pixbuf)
{
	LocationStatusMenuItemPrivate *p = GET_PRIVATE(obj);
	hildon_button_set_image(HILDON_BUTTON(p->status_button),
		gtk_image_new_from_pixbuf(pixbuf));
	hildon_button_set_image_position(HILDON_BUTTON(p->status_button), 0);
}

static int fix_acquiring_cb(gpointer obj)
{
	LocationStatusMenuItemPrivate *p = GET_PRIVATE(obj);

	if (p->locationdaemon_running == 0)
		return 0;

	if (p->prev_mode == LOCATION_GPS_DEVICE_MODE_NOT_SEEN)
		return 1;

	if (p->prev_mode != LOCATION_GPS_DEVICE_MODE_NO_FIX)
		return 0;

	switch (p->current_status_icon) {
	case STATUS_ICON_FOUND:
		set_status_icon(obj, p->pix18_gps_searching);
		p->current_status_icon = STATUS_ICON_SEARCHING;
		break;
	case STATUS_ICON_SEARCHING:
	default:
		set_status_icon(obj, p->pix18_gps_location);
		p->current_status_icon = STATUS_ICON_FOUND;
		break;
	}

	return 1;
}

static int fix_acquired(gpointer obj)
{
	LocationStatusMenuItemPrivate *p = GET_PRIVATE(obj);
	set_status_icon(obj, p->pix18_gps_location);
	p->current_status_icon = STATUS_ICON_FOUND;
	return 0;
}

static int handle_fixstatus(gpointer obj, DBusMessage *msg)
{
	LocationStatusMenuItemPrivate *p = GET_PRIVATE(obj);

	dbus_message_get_args(msg, NULL, DBUS_TYPE_BYTE,
		&p->curr_mode, DBUS_TYPE_INVALID);

	if (p->curr_mode == p->prev_mode)
		return 1;

	p->prev_mode = p->curr_mode;

	/* Either static icon, or blink cb */
	switch (p->curr_mode) {
	case LOCATION_GPS_DEVICE_MODE_NO_FIX:
		g_timeout_add_seconds(1, fix_acquiring_cb, obj);
		break;
	case LOCATION_GPS_DEVICE_MODE_2D:
	case LOCATION_GPS_DEVICE_MODE_3D:
		fix_acquired(obj);
		break;
	default:
		break;
	};

	return 1;
}

static int handle_running(gpointer obj, DBusMessage *msg)
{
	/* Either show or hide status icon */
	LocationStatusMenuItemPrivate *p = GET_PRIVATE(obj);

	dbus_message_get_args(msg, NULL, DBUS_TYPE_BYTE,
		&p->locationdaemon_running, DBUS_TYPE_INVALID);

	switch (p->locationdaemon_running) {
	case 0:
		/* Stopped */
		set_status_icon(obj, NULL);
		set_button_icon(obj, p->pix48_gps_not_connected);
		gtk_widget_hide_all(GTK_WIDGET(obj));
		break;
	case 1:
		/* Started */
		set_status_icon(obj, p->pix18_gps_not_connected);
		set_button_icon(obj, p->pix48_gps_location);
		gtk_widget_show_all(GTK_WIDGET(obj));
		break;
	default:
		break;
	}

	return 1;
}

static int on_locationdaemon_signal(DBusConnection *dbus, DBusMessage *msg, gpointer obj)
{
	(void)dbus;

	if (dbus_message_is_signal(msg, "org.maemo.LocationDaemon.Device",
			"FixStatusChanged"))
		return handle_fixstatus(obj, msg);

	if (dbus_message_is_signal(msg, "org.maemo.LocationDaemon.Running",
			"Running"))
		return handle_running(obj, msg);

	return 1;
}

static void location_status_menu_item_init(LocationStatusMenuItem *self)
{
	LocationStatusMenuItemPrivate *p = GET_PRIVATE(self);
	GtkIconTheme *theme;
	DBusError err;

	dbus_error_init(&err);

	p->locationdaemon_running = 0;
	p->prev_mode = LOCATION_GPS_DEVICE_MODE_NOT_SEEN;
	p->curr_mode = LOCATION_GPS_DEVICE_MODE_NOT_SEEN;

	p->dbus = hd_status_plugin_item_get_dbus_connection(
		HD_STATUS_PLUGIN_ITEM(self),
		DBUS_BUS_SYSTEM, &err);

	if (dbus_error_is_set(&err)) {
		status_debug("location-sb: Error getting dbus system bus: %s",
			err.message);
		dbus_error_free(&err);
		return;
	}

	dbus_connection_setup_with_g_main(p->dbus, NULL);

	dbus_bus_add_match(p->dbus,
		"type='signal',interface='org.maemo.LocationDaemon.Running',member='Running'",
		&err);

	if (dbus_error_is_set(&err)) {
		status_debug("location-sb: Failed to add match: %s", err.message);
		dbus_error_free(&err);
		return;
	}

	dbus_bus_add_match(p->dbus,
		"type='signal',interface='org.maemo.LocationDaemon.Device',member='FixStatusChanged'",
		&err);

	if (dbus_error_is_set(&err)) {
		status_debug("location-sb: Failed to add match: %s", err.message);
		dbus_error_free(&err);
		return;
	}

	if (!dbus_connection_add_filter(p->dbus,
			(DBusHandleMessageFunction)on_locationdaemon_signal,
			self, NULL)) {
		status_debug("location-sb: Failed to add dbus filter");
		return;
	}

	theme = gtk_icon_theme_get_default();
	p->pix18_gps_searching = gtk_icon_theme_load_icon(theme,
		"gps_searching", 18, 0, NULL);
	p->pix18_gps_location = gtk_icon_theme_load_icon(theme,
		"gps_location", 18, 0, NULL);
	p->pix18_gps_not_connected = gtk_icon_theme_load_icon(theme,
		"gps_not_connected", 18, 0, NULL);
	p->pix48_gps_location = gtk_icon_theme_load_icon(theme,
		"gps_location", 48, 0, NULL);
	p->pix48_gps_not_connected = gtk_icon_theme_load_icon(theme,
		"gps_not_connected", 48, 0, NULL);

	p->osso = osso_initialize("location-sb", VERSION, FALSE, NULL);

	set_status_icon(self, NULL);

	p->status_button = hildon_button_new_with_text(
		HILDON_SIZE_FINGER_HEIGHT,
		HILDON_BUTTON_ARRANGEMENT_VERTICAL,
		g_dgettext("osso-location-ui", "loca_fi_status_menu_location"),
		NULL);

	hildon_button_set_alignment(HILDON_BUTTON(p->status_button),
		0.0, 0.5, 0.0, 0.0);

	hildon_button_set_style(HILDON_BUTTON(p->status_button),
		 HILDON_BUTTON_STYLE_PICKER);
	set_button_icon(self, p->pix48_gps_not_connected);

	g_signal_connect_data(p->status_button, "clicked",
		G_CALLBACK(execute_cp_plugin), self, 0, 0);

	gtk_container_add(GTK_CONTAINER(self), p->status_button);
}

static void location_status_menu_item_finalize(GObject *obj)
{
	LocationStatusMenuItemPrivate *p = GET_PRIVATE(obj);

	if (p->dbus) {
		dbus_bus_remove_match(p->dbus,
			"type='signal',interface='org.maemo.LocationDaemon.Device',member='FixStatusChanged'",
			NULL);
		dbus_bus_remove_match(p->dbus,
			"type='signal',interface='org.maemo.LocationDaemon.Running',member='Running'",
			NULL);
		dbus_connection_remove_filter(p->dbus,
			(DBusHandleMessageFunction)on_locationdaemon_signal, obj);
		dbus_connection_unref(p->dbus);
		p->dbus = NULL;
	}

	if (p->osso)
		osso_deinitialize(p->osso);

	G_OBJECT_CLASS(location_status_menu_item_parent_class)->finalize(obj);
}

static void location_status_menu_item_class_init(LocationStatusMenuItemClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = location_status_menu_item_finalize;
}

static void location_status_menu_item_class_finalize(LocationStatusMenuItemClass *klass)
{
	(void)klass;
}
