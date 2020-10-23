/*
 * Copyright (c) 2020 Ivan J. <parazyd@dyne.org>
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

#include <libintl.h>
#include <gtk/gtk.h>
#include <hildon/hildon.h>
#include <libhildondesktop/libhildondesktop.h>
#include <libosso.h>

#define LOCATION_STATUSBAR_PLUGIN_TYPE (location_statusbar_plugin_get_type())
#define LOCATION_STATUSBAR_PLUGIN(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
			LOCATION_STATUSBAR_PLUGIN_TYPE, LocationStatusbarPlugin))

typedef struct _LocationStatusbarPlugin LocationStatusbarPlugin;
typedef struct _LocationStatusbarPluginClass LocationStatusbarPluginClass;
typedef struct _LocationStatusbarPluginPrivate LocationStatusbarPluginPrivate;

struct _LocationStatusbarPlugin
{
	HDStatusMenuItem parent;
	LocationStatusbarPluginPrivate *priv;
};

struct _LocationStatusbarPluginClass
{
	HDStatusMenuItemClass parent;
};

/* XXX: TODO: Get this from liblocation-dev */
typedef enum {
	LOCATION_GPS_DEVICE_STATUS_NO_FIX,
	LOCATION_GPS_DEVICE_STATUS_FIX,
} LocationGPSDeviceStatus;

struct _LocationStatusbarPluginPrivate
{
	osso_context_t *osso;
	GtkWidget *status_menu_button;
	GtkWidget *status_area_icon;
	GtkWidget *status_menu_icon;
	LocationGPSDeviceStatus gps_status;
};


HD_DEFINE_PLUGIN_MODULE_EXTENDED(LocationStatusbarPlugin, location_statusbar_plugin,
		HD_TYPE_STATUS_MENU_ITEM, G_ADD_PRIVATE_DYNAMIC(LocationStatusbarPlugin),,);

static void start_cp_applet(GtkWidget *button, LocationStatusbarPlugin *self)
{
	LocationStatusbarPluginPrivate *priv = location_statusbar_plugin_get_instance_private(self);

	if (osso_cp_plugin_execute(priv->osso, "liblocation_applet.so", NULL, TRUE) == -1)
		g_log(NULL, G_LOG_LEVEL_WARNING, "location-sb: Error starting location applet");
}

static void location_statusbar_plugin_set_status_icon(gpointer data, GdkPixbuf *pixbuf)
{
	LocationStatusbarPlugin *item = LOCATION_STATUSBAR_PLUGIN(data);
	g_return_if_fail(item != NULL && pixbuf != NULL);
	hd_status_plugin_item_set_status_area_icon(HD_STATUS_PLUGIN_ITEM(item), pixbuf);
	pixbuf = NULL;
}

static void location_statusbar_plugin_set_menu_icon(LocationStatusbarPlugin *self, GdkPixbuf *pixbuf)
{
	LocationStatusbarPluginPrivate *priv = location_statusbar_plugin_get_instance_private(self);
	priv->status_menu_icon = gtk_image_new_from_pixbuf(pixbuf);
	hildon_button_set_image(HILDON_BUTTON(priv->status_menu_button), priv->status_menu_icon);
	pixbuf = NULL;
}

static void location_statusbar_plugin_set_icons(LocationStatusbarPlugin *self)
{
	LocationStatusbarPluginPrivate *priv = location_statusbar_plugin_get_instance_private(self);
	GdkPixbuf *pixbuf = NULL;
	GtkIconTheme *theme = gtk_icon_theme_get_default();

	/* TODO: There is searching/notconnected/location */
	if (priv->gps_status == LOCATION_GPS_DEVICE_STATUS_NO_FIX)
		pixbuf = gtk_icon_theme_load_icon(theme, "gps_searching", 16, 0, NULL);
	else
		pixbuf = gtk_icon_theme_load_icon(theme, "gps_location", 16, 0, NULL);

	location_statusbar_plugin_set_status_icon(self, pixbuf);

	if (priv->gps_status == LOCATION_GPS_DEVICE_STATUS_NO_FIX)
		pixbuf = gtk_icon_theme_load_icon(theme, "gps_not_connected", 48, 0, NULL);
	else
		pixbuf = gtk_icon_theme_load_icon(theme, "gps_location", 48, 0, NULL);

	location_statusbar_plugin_set_menu_icon(self, pixbuf);

	g_object_unref(pixbuf);
}

static void location_statusbar_plugin_init(LocationStatusbarPlugin *self)
{
	LocationStatusbarPluginPrivate *priv = location_statusbar_plugin_get_instance_private(self);
	GtkIconTheme *theme = gtk_icon_theme_get_default();

	self->priv = priv;
	priv->osso = osso_initialize("location-sb", "0.106", FALSE, NULL);
	priv->status_area_icon = NULL;
	priv->status_menu_icon = NULL;
	priv->status_menu_button = hildon_button_new(
			HILDON_SIZE_FINGER_HEIGHT,
			HILDON_BUTTON_ARRANGEMENT_VERTICAL);

	hildon_button_set_style(HILDON_BUTTON(priv->status_menu_button),
			HILDON_BUTTON_STYLE_PICKER);
	hildon_button_set_alignment(HILDON_BUTTON(priv->status_menu_button),
			0.0, 0.5, 0.0, 0.0);
	hildon_button_set_title(HILDON_BUTTON(priv->status_menu_button),
			dgettext("osso-location-ui", "loca_fi_status_menu_location"));

	location_statusbar_plugin_set_icons(self);

	g_signal_connect(priv->status_menu_button, "clicked",
			G_CALLBACK(start_cp_applet), self);

	gtk_container_add(GTK_CONTAINER(self), priv->status_menu_button);
	gtk_widget_show_all(GTK_WIDGET(self));
}

static void location_statusbar_plugin_finalize(GObject *self)
{
	LocationStatusbarPlugin *plugin = LOCATION_STATUSBAR_PLUGIN(self);
	LocationStatusbarPluginPrivate *priv = plugin->priv;

	/* location_status_menu_show(plugin, FALSE, FALSE); */

	if (priv) {
		g_object_unref(priv->status_menu_button);
		if (priv->status_area_icon)
			g_object_unref(priv->status_area_icon);
		if (priv->status_menu_icon)
			g_object_unref(priv->status_menu_icon);
		osso_deinitialize(priv->osso);
	}
}

static void location_statusbar_plugin_class_init(LocationStatusbarPluginClass *class)
{
	return;
}

static void location_statusbar_plugin_class_finalize(LocationStatusbarPluginClass *class)
{
	return;
}

LocationStatusbarPlugin *location_statusbar_plugin_new(void)
{
	return g_object_new(LOCATION_STATUSBAR_PLUGIN_TYPE, NULL);
}
