/*
** ucview_plugin.h
** 
*/

/*
  Copyright (C) 2007  Arne Caspari <arne@unicap-imaging.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef   	UCVIEW_PLUGIN_H_
# define   	UCVIEW_PLUGIN_H_

#include <unicap.h>
#include <glib.h>
#include <gtk/gtk.h>

#include "ucview-window.h"

struct _UCViewPlugin
{
      gchar *name;
      gchar *description;
      gchar **authors;
      gchar *copyright;
      gchar *license;
      gchar *version;
      gchar *website;      
      
      gpointer user_ptr;
      unicap_new_frame_callback_t new_frame_cb;
      unicap_new_frame_callback_t record_frame_cb;
      unicap_new_frame_callback_t display_frame_cb;
};

typedef struct _UCViewPlugin UCViewPlugin;

typedef guint (*UCViewPluginGetApiVersionFunc)(void);
typedef void (*UCViewPluginInitFunc)(UCViewWindow *ucv, UCViewPlugin *plugin, const gchar *plugin_path );
typedef void (*UCViewPluginUnloadFunc)(UCViewPlugin *plugin);
typedef void (*UCViewPluginDisableFunc)(UCViewPlugin *plugin);
typedef void (*UCViewPluginEnableFunc)(UCViewPlugin *plugin, unicap_format_t *format);
typedef GtkWidget *(*UCViewPluginConfigureFunc)(UCViewPlugin *plugin);

#endif 	    /* !UCVIEW_PLUGIN_H_ */
