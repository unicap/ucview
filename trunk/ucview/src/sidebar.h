/*
** sidebar.h
** 
** Made by Arne Caspari
** Login   <arne@localhost>
** 
** Started on  Mon Apr  7 07:32:33 2008 Arne Caspari
** Last update Mon Apr  7 07:32:33 2008 Arne Caspari
*/

#ifndef   	SIDEBAR_H_
# define   	SIDEBAR_H_

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <unicap.h>
#include "ucview-window.h"

GType sidebar_get_type( void );
GtkWidget *sidebar_new( UCViewWindow *ucv );

#define SIDEBAR_TYPE              (sidebar_get_type())
#define SIDEBAR(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj), SIDEBAR_TYPE, Sidebar))
#define SIDEBAR_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass), SIDEBAR_TYPE, SidebarClass))
#define IS_SIDEBAR(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj), SIDEBAR_TYPE))
#define IS_SIDEBAR_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE((klass), SIDEBAR_TYPE))
#define SIDEBAR_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), SIDEBAR_TYPE, SidebarClass))


typedef struct _Sidebar Sidebar;
typedef struct _SidebarClass SidebarClass;
typedef struct _SidebarPrivate SidebarPrivate;

struct _Sidebar
{
      GtkVBox parent;

      /*< private >*/
      SidebarPrivate *priv;
};

struct _SidebarClass
{
      GtkVBoxClass parent_class;
      void (* response)( Sidebar *box, gint response_id );
};

gboolean sidebar_add_image( Sidebar *sidebar, unicap_data_buffer_t *image, gchar *path );

#endif 	    /* !SIDEBAR_H_ */
