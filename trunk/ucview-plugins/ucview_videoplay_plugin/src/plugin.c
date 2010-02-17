/*
** plugin.c
** 
** UCView Videoplay Plugin
**
*/

#include <ucview_plugin.h>
#include <ucview-window.h>
#include <unicap.h>
#include <glib/gi18n.h>
#include <glib.h>
#include <string.h>
#include <math.h>

#include <gst/gst.h>
#include <gst/interfaces/xoverlay.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include "ucvidplay.h"


static gboolean enable_logging = ENABLE_LOGGING;


static void enable_cb( GtkAction *action, UCViewPlugin *plugin );
static void display_image_cb( UCViewWindow *ucv, unicap_data_buffer_t *buffer, UCViewPlugin *plugin );

static void video_file_created_cb( UCViewWindow *ucv, gchar *path, UCViewPlugin *plugin );


struct private_data
{
      UCViewWindow *ucv;
      
      guint ui_merge_id;
      GtkActionGroup *actions;

      gboolean active;

      GtkWidget *player;


      gchar *path;
};


gchar *authors[] = 
{
   "Arne Caspari", 
   NULL
};

UCViewPlugin default_plugin_data = 
{
   "Videoplay",                     // Name
   "Videoplay Plugin",              // Description
   authors,                        // Authors
   "Copyright 2007-2009 Arne Caspari",  // Copyright
   "GPL",                          // License
   "1.1",                          // Version
   "http://www.unicap-imaging.org",// Website

   NULL,                           // user_ptr

   NULL, 
   NULL,                           // record_frame_cb
   NULL,                           // display_frame_cb
};


static GtkActionEntry action_entries[] = 
{
   { "EnablePlayback", NULL, N_("Play last recording"), NULL, N_("Play last recording"), 
     G_CALLBACK( enable_cb ) }, 
};
static const guint n_action_entries = G_N_ELEMENTS( action_entries );






static void log_func( const gchar *domain, GLogLevelFlags log_level, const gchar *message, gpointer data )
{
   if( enable_logging )
   {
      g_log_default_handler( domain, log_level, message, data );
   }
}

/*
  ==================================================================================
                        UCView Plugin Interface Implementation
  ==================================================================================
*/


guint ucview_plugin_get_api_version()
{
   return 1<<16;
}

/**
 * ucview_plugin_init: Called when the plugin gets loaded
 *
 */
void ucview_plugin_init( UCViewWindow *ucv, UCViewPlugin *plugin )
{
   struct private_data *data = g_malloc0( sizeof( struct private_data ) );
   
   g_log_set_handler( LOG_DOMAIN, G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION,
		      log_func, NULL );
   g_log( LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Videoplay plugin init\n" );
   g_memmove( plugin, &default_plugin_data, sizeof( UCViewPlugin ) );
   plugin->user_ptr = data;   

   data->actions = gtk_action_group_new( "Videoplay Plugin Actions" );
   gtk_action_group_add_actions( data->actions, action_entries, n_action_entries, plugin );
   
   data->ucv = ucv;
   data->active = FALSE;
   
   data->path = NULL;
#if 1
   data->path = g_strdup( "/home/arne/tmp/ucview/Video_0009.ogg" );
#endif

   gst_init_check( NULL, NULL, NULL );
}


/**
 * ucview_plugin_unload: Called when the plugin gets unloaded
 *
 */
void ucview_plugin_unload( UCViewPlugin *plugin )
{
   struct private_data *data = plugin->user_ptr;
   
   if( data->path )
   {
      g_free( data->path );
   }

   g_free( data );
}

/**
 * ucview_plugin_enable: Called when the plugin gets enabled via the Settings/PlugIns dialog
 *
 */
void ucview_plugin_enable( UCViewPlugin *plugin )
{
   struct private_data *data = plugin->user_ptr;
   GtkUIManager *ui;   

   g_log( LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Videoplay plugin enable\n" );

   ui = ucview_window_get_ui_manager( UCVIEW_WINDOW( data->ucv ) );   
   data->ui_merge_id = gtk_ui_manager_new_merge_id( ui );
   gtk_ui_manager_add_ui( ui, data->ui_merge_id, 
			  "ui/MenuBar/ImageMenu", "Play last recording", "EnablePlayback", 
			  GTK_UI_MANAGER_MENUITEM, FALSE );
   gtk_ui_manager_insert_action_group( ui, data->actions, 0 );

   data->player = uc_video_player_new();
   g_signal_connect( data->player, "delete-event", (GCallback)gtk_widget_hide_on_delete, NULL );

   g_signal_connect( data->ucv, "video-file-created", (GCallback)video_file_created_cb, plugin );
}

/**
 * ucview_plugin_disable: Called when the plugin gets disabled via the Settings/PlugIns dialog
 *
 */
void ucview_plugin_disable( UCViewPlugin *plugin )
{
   struct private_data *data = plugin->user_ptr;
   GtkUIManager *ui;   

   g_log( LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Videoplay plugin disable\n" );

   ui = ucview_window_get_ui_manager( UCVIEW_WINDOW( data->ucv ) );   
   gtk_ui_manager_remove_ui( ui, data->ui_merge_id );
   gtk_ui_manager_remove_action_group( ui, data->actions );

   gtk_widget_destroy( data->player );

}

/**
 * ucview_plugin_configure: Called when the configure button gets clicked in the plugins dialog
 *
 */
GtkWidget *ucview_plugin_configure( UCViewPlugin *plugin )
{
   return NULL;
}

/*
  ==================================================================================
                               Plugin Implementation
  ==================================================================================
*/



static void enable_cb( GtkAction *action, UCViewPlugin *plugin )
{
   struct private_data *data = plugin->user_ptr;
   
   if( data->path )
   {
      gtk_window_present( GTK_WINDOW( data->player ) );
      uc_video_player_play_file( UC_VIDEO_PLAYER( data->player ), data->path );
   }

}

static void video_file_created_cb( UCViewWindow *ucv, gchar *path, UCViewPlugin *plugin )
{
   struct private_data *data = plugin->user_ptr;

   g_log( LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Videoplay: video file created: %s", path );
   if( data->path )
   {
      g_free( data->path );
   }
   
   data->path = g_strdup( path );
}

