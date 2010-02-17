/*
** plugin_loader.c
** 
*/

/*
  Copyright (C) 2007-2009  Arne Caspari <arne@unicap-imaging.org>

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

#include <glib.h>
#include <glib/gstdio.h>
#include <gconf/gconf-client.h>
#include <unicapgtk.h>

#include "ucview_plugin.h"
#include "plugin_loader.h"
#include "ucview.h"

static gchar *get_user_plugin_path( void );



void ucview_load_plugins( UCViewWindow *ucv )
{
   gchar *path;
   gchar *user_path;
   gchar *search_paths[3];

   int i = 0;


   if( !g_module_supported() )
   {
      return;
   }

   user_path = get_user_plugin_path();
   if( user_path )
      search_paths[ i++ ] = user_path;
   search_paths[ i++ ] = UCVIEW_PLUGINDIR;
   search_paths[ i++ ] = NULL;
   
   for( i = 1, path = search_paths[ 0 ]; path; path = search_paths[ i++ ] )
   {
   
      if( path )
      {
	 GDir *dir;
	 
	 dir = g_dir_open( path, 0, NULL );
	 g_printf( "search module path: %s\n", path );
	 
	 if( dir )
	 {
	    const gchar *name;
	    
	    while( ( name = g_dir_read_name( dir ) ) )
	    {
	       if( g_str_has_suffix( name, G_MODULE_SUFFIX ) )
	       {
		  gchar *full_path;
		  GModule *module;
		  
		  full_path = g_build_path( G_DIR_SEPARATOR_S, path, name, NULL );
		  module = g_module_open( full_path, G_MODULE_BIND_LOCAL );
		  g_printf( "open module: %s\n", full_path );
		  g_free( full_path );
	       
		  if( module )
		  {
		     UCViewPluginInitFunc init_func;
		     UCViewPluginGetApiVersionFunc get_api_ver_func;

		     if( !g_module_symbol( module, "ucview_plugin_get_api_version", (void*)&get_api_ver_func ) ){
			g_warning( "no symbol: get_api_version (%s)", name );
			continue;
		     }
		     
		     if( ( get_api_ver_func()>>16 & 0xff ) != 1 ){
			g_warning( "wrong api_version (%s) %x", name, get_api_ver_func() );
			continue;
		     }
		     
		     if( g_module_symbol( module, "ucview_plugin_init", (void*)&init_func ) )
		     {
			UCViewPluginData *plugin;
			gchar *gconf_path;
			gchar *keyname = g_path_get_basename( g_module_name( module ) );
			
			gconf_path = g_build_path( "/", UCVIEW_GCONF_DIR, "plugins", keyname, NULL );
			
			g_free( keyname );
			
			plugin = g_malloc( sizeof( UCViewPluginData ) );
			plugin->plugin_data = g_malloc( sizeof( UCViewPlugin ) );
			
			
			plugin->client = ucv->client;
			plugin->enable = gconf_client_get_bool( ucv->client, gconf_path, NULL );
			
			init_func( ucv, plugin->plugin_data, path );
			plugin->module = module;
			
			ucv->modules = g_list_prepend( ucv->modules, plugin );
			g_free( gconf_path );
			
		     }
		     else
		     {
			g_module_close( module );
		     }
		  } else
		  {
		     g_message( "Error loading module: %s\n", g_module_error() );
		  }
	       }
	    }
	    g_dir_close( dir );
	 }   
      } 
   }
   g_free( user_path );

}

void ucview_unload_plugins( UCViewWindow *ucv )
{
   GList *entry;

   for( entry = g_list_first( ucv->modules ); entry; entry = g_list_next( entry ) )
   {
      UCViewPluginData *plugin;
      UCViewPluginUnloadFunc unload_func;

      plugin = entry->data;
      
      if( g_module_symbol( plugin->module, "ucview_plugin_unload", (void*)&unload_func ) )
      {
	 unload_func( plugin->plugin_data );
      }
      
      g_module_close( plugin->module );
      
      g_free( plugin );
   }
   
   g_list_free( ucv->modules );
   ucv->modules = NULL;
}

void ucview_enable_plugin( UCViewWindow *window, UCViewPluginData *plugin, gboolean enable )
{
   gchar *gconf_path;
   gchar *keyname = g_path_get_basename( g_module_name( plugin->module ) );

   gconf_path = g_build_path( "/", UCVIEW_GCONF_DIR, "plugins", keyname, NULL );
   g_free( keyname );
   gconf_client_set_bool( plugin->client, gconf_path, enable, NULL );
   g_free( gconf_path );

   plugin->enable = enable;
   
   if( !enable )
   {
      UCViewPluginDisableFunc func;
      
      if( g_module_symbol( plugin->module, "ucview_plugin_disable", (void*)&func ) )
      {
	 func( plugin->plugin_data );
      }
   }
   else
   {
      UCViewPluginEnableFunc func;
      unicap_format_t format;
      
      unicapgtk_video_display_get_format( UNICAPGTK_VIDEO_DISPLAY( window->display ), &format );
      
      if( g_module_symbol( plugin->module, "ucview_plugin_enable", (void*)&func ) )
      {
	 func( plugin->plugin_data, &format );
      }
   }
}

void ucview_enable_all_plugins( UCViewWindow *window )
{
   GList *entry;   
   for( entry = g_list_first( window->modules ); entry; entry=g_list_next( entry ) )
   {
      UCViewPluginData *plugin;
      UCViewPluginEnableFunc enable_func;

      plugin = entry->data;
      if( plugin->enable )
      {
	 if( g_module_symbol( plugin->module, "ucview_plugin_enable", (void*)&enable_func ) )
	 {
	    enable_func( plugin->plugin_data, &window->format );
	 }
      }
   }
}


static gchar *get_user_plugin_path( void )
{
   const gchar *homepath;
   gchar *path;
   
   homepath = g_getenv( "HOME" );
   if( !homepath )
   {
      return NULL;
   }
   
   path = g_build_path( G_DIR_SEPARATOR_S, homepath, ".ucview", "plugins", NULL );
   if( g_file_test( path, G_FILE_TEST_EXISTS ) )
   {
      if( !g_file_test( path, G_FILE_TEST_IS_DIR ) )
      {
	 g_warning( "%s is not a directory\n", path );
	 g_free( path );
	 return NULL;
      }
   }
   else
   {
      if( g_mkdir( path, 0755 ) )
      {
	 g_warning( "Failed to create '%s'\n", path );
	 g_free( path );
	 return NULL;
      }
   }

   return path;
}
