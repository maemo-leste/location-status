/*
 * Copyright (c) 2020 Ivan J. <parazyd@dyne.org>
 *
 * This file is part of status-area-applet-location
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <dbus/dbus.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <gtk/gtk.h>
#include <libintl.h>

#include <hildon/hildon.h>
#include <libhildondesktop/libhildondesktop.h>
#include <libosso.h>
#include <location/location-gps-device.h>

typedef struct _LocationStatusMenuItem        LocationStatusMenuItem;
typedef struct _LocationStatusMenuItemClass   LocationStatusMenuItemClass;
typedef struct _LocationStatusMenuItemPrivate LocationStatusMenuItemPrivate;

struct _LocationStatusMenuItem
{
	HDStatusMenuItem parent;
	LocationStatusMenuItemPrivate *prov;
};

struct _LocationStatusMenuItemClass
{
	HDStatusMenuItemClass parent;
};

struct _LocationStatusMenuItemPrivate
{
	osso_context_t *osso;
	GtkContainer *container;
	DBusConnection *dbus;
	GdkPixbuf *pix16_gps_searching;
	GdkPixbuf *pix16_gps_location;
	GdkPixbuf *pix16_gps_not_connected;
	GdkPixbuf *pix48_gps_location;
	GdkPixbuf *pix48_gps_not_connected;
	LocationGPSDeviceMode searching;
	GtkWidget *button;
};

GType location_status_menu_item_get_type(void);

HD_DEFINE_PLUGIN_MODULE_EXTENDED(LocationStatusMenuItem,
		location_status_menu_item, HD_TYPE_STATUS_MENU_IDEM,
		G_ADD_PRIVATE_DYNAMIC(LocationStatusMenuItem), , );

static int execute_cp_plugin(GObject *object)
{
    LocationStatusMenuItem *plugin = G_TYPE_CHECK_INSTANCE_CAST(
            object, location_status_menu_item_get_type(),
            LocationStatusMenuItem);

	if (osso_cp_plugin_execute(plugin->priv->osso,
				"liblocation_applet.so", object, TRUE) == -1)
		g_warning("location-sb: Error starting location applet");
	return 0;
}

static void location_status_menu_item_init(LocationStatusMenuItem *plugin)
{
	plugin->priv = location_status_menu_item_get_instance_private(plugin);
	LocationStatusMenuItemPrivate *priv = plugin->priv;
	DBusError error;

	GtkIconTheme *theme;
	const gchar *btn_title;
	GtkWidget *button_icon;

	dbus_error_init(&error);

	priv->dbus = hd_status_plugin_item_get_dbus_connection(
			DBUS_BUS_SYSTEM, &error);

	if (dbus_error_is_set(&error)) {
		g_warning("location-sb: Error getting bus: %s", error.message);
		dbus_error_free(&error);
	} else {
		dbus_connection_setup_with_g_main(priv->dbus, NULL);
		dbus_bus_add_match(priv->dbus,
				"type='signal',interface='org.gpsd',member='fix'",
				&error);
		if (dbus_error_is_set(&error)) {
			g_warning("location-sb: Failed to add match: %s", error.message);
			dbus_error_free(&error);
		}

		if (!dbus_connection_add_filter(priv->dbus,
					(DBusHandleMessageFunction)on_gpsd_signal,
					plugin, NULL)) {
			g_warning("location-sb: Failed to add filter");
		}
	}

	theme = gtk_icon_theme_get_default();
	priv->pix16_gps_searching = gtk_icon_theme_load_icon(theme,
			"gps_searching", 16, 0, NULL);
	priv->pix16_gps_location = gtk_icon_theme_load_icon(theme,
			"gps_location", 16, 0, NULL);
	priv->pix16_gps_not_connected = gtk_icon_theme_load_icon(theme,
			"gps_not_connected", 16, 0, NULL);
	priv->pix48_gps_location = gtk_icon_theme_load_icon(theme,
			"gps_location", 48, 0, NULL);
	priv->pix48_gps_not_connected = gtk_icon_theme_load_icon(theme,
			"gps_not_connected", 48, 0, NULL);

	priv->osso = osso_initialize("location-sb", "0.106", FALSE, NULL);

	hd_status_plugin_item_set_status_area_icon(plugin, NULL);

	btn_title = d_gettext("osso-location-ui", "loca_fi_status_menu_location");
	priv->button = hildon_button_new_with_text(
			HILDON_SIZE_FINGER_HEIGHT,
			HILDON_BUTTON_ARRANGEMENT_VERTICAL,
			btn_title, NULL);

	hildon_button_set_alignment(priv->button, 0.0, 0.5, 0.0, 0.0);
	hildon_button_set_style(priv->button, HILDON_BUTTON_STYLE_PICKER);
	button_icon = gtk_image_new_from_pixbuf(priv->pix48_gps_location);
	hildon_button_set_image(priv->button, button_icon);
	hildon_button_set_image_position(priv->button, 0);

	g_signal_connect_data(priv->button, "clicked",
			(GCallback)execute_cp_plugin, plugin, 0, 0);

	gtk_container_add(GTK_CONTAINER(plugin), priv->button);
	gtk_widget_show_all(GTK_WIDGET(plugin));

	// connect set_icon_if_visible to something
	fix_status_changed(plugin);
}

static void location_status_menu_item_finalize(GObject *object)
{
	LocationStatusMenuItem *plugin = G_TYPE_CHECK_INSTANCE_CAST(
			object, location_status_menu_item_get_type(),
			LocationStatusMenuItem);

	if (plugin->priv->osso)
		osso_deinitialize(&plugin->priv->osso);

	if (plugin->priv->dbus) {
		dbus_bus_remove_match(plugin->priv->dbus,
				"type='signal',interface='org.gpsd',member='fix'"
				NULL);
		dbus_connection_remove_filter(plugin->priv->dbus,
				(DBusHandleMessageFunction)on_gpsd_signal,
				object);
		dbus_connection_unref(plugin->priv->dbus);
		plugin->priv->dbus = NULL;
	}

	G_OBJECT_CLASS(location_status_menu_item_parent_class)->finalize(object);
}

static void location_status_menu_item_class_finalize(LocationStatusMenuItemClass *klass)
{
	return;
}

static void location_status_menu_item_class_init(LocationStatusMenuItemClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = location_status_menu_item_class_finalize;
}
