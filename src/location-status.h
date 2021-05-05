#ifndef LOCATION_APPLET_H
#define LOCATION_APPLET_H

#include <config.h>

#include <glib.h>
#include <glib-object.h>
#include <libhildondesktop/libhildondesktop.h>
#include <hildon/hildon.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <libosso.h>

G_BEGIN_DECLS


#define LOCATION_STATUS_MENU_ITEM_TYPE (location_status_menu_item_get_type())
#define LOCATION_STATUS(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), LOCATION_STATUS_MENU_ITEM_TYPE, LocationStatusMenuItem))
#define LOCATION_STATUS_MENU_ITEM_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), LOCATION_STATUS_MENU_ITEM_TYPE, LocationStatusMenuItemPrivate))

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
	HDStatusMenuItemClass parent_class;
};

typedef enum {
	STATUS_ICON_NOT_CONNECTED,
	STATUS_ICON_SEARCHING,
	STATUS_ICON_FOUND
} CurStatusIcon;

GType location_status_menu_item_get_type(void);

G_END_DECLS

#endif /* LOCATION_APPLET_H */
