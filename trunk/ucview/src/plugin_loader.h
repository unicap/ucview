/*
** plugin_loader.h
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

#ifndef   	PLUGIN_LOADER_H_
# define   	PLUGIN_LOADER_H_

#include <gconf/gconf-client.h>

#include "ucview.h"
#include "ucview_plugin.h"

struct _UCViewPluginData
{
      UCViewPlugin *plugin_data;
      GModule *module;
      gboolean enable;
      GConfClient *client;
};

typedef struct _UCViewPluginData UCViewPluginData;


void ucview_load_plugins( UCViewWindow *ucv );
void ucview_unload_plugins( UCViewWindow *ucv );
void ucview_enable_plugin( UCViewWindow *ucv, UCViewPluginData *plugin, gboolean enable );
void ucview_enable_all_plugins( UCViewWindow *window );

#endif 	    /* !PLUGIN_LOADER_H_ */
