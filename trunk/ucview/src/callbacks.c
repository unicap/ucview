/*
** callbacks.c
** 
** Made by (Arne Caspari)
** Login   <arne@arne-laptop>
** 
** Started on  Wed Feb 21 18:18:28 2007 Arne Caspari
** Last update Wed Mar 21 18:30:17 2007 Arne Caspari
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

#define _GNU_SOURCE
#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gconf/gconf-client.h>

#include <unicap.h>
#include <unicapgtk.h>
#include <ucil.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ucview.h"
#include "plugin_loader.h"
#include "callbacks.h"
#include "ucview-device-dialog.h"
#include "user_config.h"
#include "ucview-info-box.h"
#include "worker.h"



void gconf_notification_cb( GConfClient *client, guint cnxn_id, GConfEntry *entry, UCViewWindow *window )
{
   const gchar *key = gconf_entry_get_key( entry );
   
   if( g_str_has_suffix( key, "show_fps" ) )
   {
      GtkAction *action = gtk_ui_manager_get_action( ucview_window_get_ui_manager( UCVIEW_WINDOW( window ) ), "/MenuBar/ImageMenu/ShowFPS" );
      gboolean active = gconf_client_get_bool( window->client, UCVIEW_GCONF_DIR "/show_fps", NULL );
      

      gtk_action_set_sensitive( action, FALSE );
      gtk_toggle_action_set_active( GTK_TOGGLE_ACTION( action ), active );
      gtk_action_set_sensitive( action, TRUE );

      ucview_window_enable_fps_display( window, active );
   }
}

gboolean fs_toolbar_enter_notify_cb( GtkWidget *widget, GdkEventCrossing *event, UCViewWindow *window )
{
   if( window->is_fullscreen )
   {
      if( window->fullscreen_timeout > 0 )
      {
	 g_source_remove( window->fullscreen_timeout );
	 window->fullscreen_timeout = 0;
      }
   }

   return FALSE;
}




void fps_toggled_cb( GtkAction *action, UCViewWindow *window )
{
   gconf_client_set_bool( window->client, UCVIEW_GCONF_DIR "/show_fps", gtk_toggle_action_get_active( GTK_TOGGLE_ACTION( action ) ), NULL );
}



void record_encode_frame_cb( unicap_event_t event, unicap_handle_t handle, unicap_data_buffer_t *buffer, UCViewWindow *window )
{
   GList *e;
   
   for( e = g_list_first( window->modules ); e; e = g_list_next( e ) )
   {
      UCViewPluginData *plugin;
      
      plugin = e->data;
      
      if( plugin->enable && plugin->plugin_data->record_frame_cb )
      {
	 plugin->plugin_data->record_frame_cb( event, handle, buffer, plugin->plugin_data->user_ptr );
      }
   }
}

static void hide_video_info_box_toggled_cb( GtkToggleButton *button, UCViewWindow *window )
{
   gconf_client_set_bool( window->client, 
			  UCVIEW_GCONF_DIR "/hide_video_saved_box", 
			  gtk_toggle_button_get_active( button ), 
			  NULL );
}

static gboolean process_video_update_progressbar_timeout( UCViewWindow *window )
{
   if( window->video_processing_progressbar )
   {
      gtk_progress_bar_set_fraction( GTK_PROGRESS_BAR( window->video_processing_progressbar ), window->video_processing_fract );
   }

   return( window->video_processing_fract < 1.0 );
}
      
static void process_video_update_func( UCViewWindow *window, gdouble fract )
{
   window->video_processing_fract = fract;
}


/**
 * process_video_worker: Worker thread function to combine the AV file
 * 
 *
 **/
static void process_video_worker( UCViewWindow *window )
{
   guint timeout;

   timeout = g_timeout_add( 250, (GSourceFunc)process_video_update_progressbar_timeout, window );
   ucil_combine_av_file( window->last_video_file, "ogg/theora", 1, (ucil_processing_info_func_t)process_video_update_func, window, NULL );
   g_source_remove( timeout );
}


void time_lapse_toggled_cb( GtkAction *action, UCViewWindow *window )
{
   if( gtk_toggle_action_get_active( GTK_TOGGLE_ACTION( action ) ) )
   {
      gint mode = gconf_client_get_int( window->client, UCVIEW_GCONF_DIR "/time_lapse_action", NULL );
      
      window->time_lapse_mode = mode;
      window->time_lapse_timer = g_timer_new();
      window->time_lapse_total_timer = g_timer_new();
      g_timer_start( window->time_lapse_timer );
      g_timer_start( window->time_lapse_total_timer );

      window->time_lapse = TRUE;
      window->time_lapse_delay = (uint)gconf_client_get_int( window->client, UCVIEW_GCONF_DIR "/time_lapse_delay", NULL );
      window->time_lapse_total_time = 0;
      window->time_lapse_total_frames = 0;
      window->time_lapse_frames = 0;
      window->time_lapse_first_frame = TRUE;

      g_signal_emit( G_OBJECT( window ), ucview_signals[ UCVIEW_TIME_LAPSE_START_SIGNAL ], 0 );

      if( gconf_client_get_bool( window->client, UCVIEW_GCONF_DIR "/time_lapse_auto_stop", NULL ) )
      {
	 if( gconf_client_get_int( window->client, UCVIEW_GCONF_DIR "/time_lapse_stop_condition", NULL ) == UCVIEW_STOP_CONDITION_TIME )
	 {
	    window->time_lapse_total_time = (uint)gconf_client_get_int( window->client, UCVIEW_GCONF_DIR "/time_lapse_time", NULL );
	 }
	 else
	 {
	    window->time_lapse_total_frames = (uint)gconf_client_get_int( window->client, UCVIEW_GCONF_DIR "/time_lapse_frames", NULL );
	 }
      }
      
      if( mode ==  UCVIEW_TIME_LAPSE_VIDEO )
      {
	 GtkAction *record_action;
	 record_action = gtk_ui_manager_get_action( ucview_window_get_ui_manager( UCVIEW_WINDOW( window ) ), "/MenuBar/EditMenu/RecordVideo" );
	 if( gtk_toggle_action_get_active( GTK_TOGGLE_ACTION( record_action ) ) )
	 {
	    gtk_toggle_action_set_active( GTK_TOGGLE_ACTION( record_action ), FALSE );
	 }
	 gtk_toggle_action_set_active( GTK_TOGGLE_ACTION( record_action ), TRUE );
	 gtk_action_set_sensitive( GTK_ACTION( record_action ), FALSE );
      }
   }
   else
   {
      window->time_lapse = FALSE;
      
      if( window->time_lapse_mode == UCVIEW_TIME_LAPSE_VIDEO )
      {
	 GtkAction *record_action;
	 record_action = gtk_ui_manager_get_action( ucview_window_get_ui_manager( UCVIEW_WINDOW( window ) ), "/MenuBar/EditMenu/RecordVideo" );
	 gtk_toggle_action_set_active( GTK_TOGGLE_ACTION( record_action ), FALSE );
	 gtk_action_set_sensitive( GTK_ACTION( record_action ), TRUE );
      }

      g_timer_destroy( window->time_lapse_timer );
      g_timer_destroy( window->time_lapse_total_timer );
   }
	 
}

void record_toggled_cb( GtkAction *action, UCViewWindow *window )
{
   if( gtk_toggle_action_get_active( GTK_TOGGLE_ACTION( action ) ) )
   {
      gchar *path;
      int i;
      gchar *filename;
      GtkAction *play_video_action;

      gchar *codec = NULL;
      
      play_video_action = gtk_ui_manager_get_action( ucview_window_get_ui_manager( UCVIEW_WINDOW( window ) ), "/MenuBar/EditMenu/PlayVideo" );
      if( play_video_action )
      {
	 gtk_toggle_action_set_active( GTK_TOGGLE_ACTION( play_video_action ), FALSE );
      }
      
      path = gconf_client_get_string( window->client, UCVIEW_GCONF_DIR "/video_file_path", NULL );
      if( !g_file_test( path, ( G_FILE_TEST_IS_DIR ) ) )
      {
	 g_warning( "Invalid path: %s\n", path );
	 return;
      }

      codec = gconf_client_get_string( window->client, UCVIEW_GCONF_DIR "/video_encoder", NULL );

      if( !window->video_filename )
      {
	 for( i = 0; i < 99999; i++ )
	 {
	    char num[5];
	    gchar *tmpname;
	    sprintf( num, "%04d", i );
	    tmpname = g_strconcat( "Video_", num, ".", ucil_get_video_file_extension( codec ), NULL );
	    filename = g_build_path( G_DIR_SEPARATOR_S, path, tmpname, NULL );
	    g_free( tmpname );
	    if( !g_file_test( filename, G_FILE_TEST_EXISTS ) )
	    {
	       break;
	    }
	 }
      }
      else
      {
	 filename = g_build_path( G_DIR_SEPARATOR_S, path, window->video_filename, NULL );
	 g_free( window->video_filename );
	 window->video_filename = NULL;
      }

      g_signal_emit( G_OBJECT( window ), ucview_signals[ UCVIEW_RECORD_START_SIGNAL ], 0, codec, filename );
      
      if( !codec || !strcmp( codec, "ogg/theora" ) )
      {
	 gint bitrate;
	 gint quality;
	 double fps;
	 gint downsize = 1;
	 guint backend_fourcc;
	 
	 bitrate = gconf_client_get_int( window->client, UCVIEW_GCONF_DIR "/video_theora_bitrate", NULL );
	 if( ( bitrate < 100 ) || ( bitrate > 10000 ) )
	 {
	    bitrate = 800000;
	 }
	 else
	 {
	    bitrate *= 1000;
	 }
	 
	 quality = gconf_client_get_int( window->client, UCVIEW_GCONF_DIR "/video_theora_quality", NULL );
	 if( ( quality < 0 ) || ( quality > 63 ) )
	 {
	    quality = 63;
	 }
	 
	 fps = gconf_client_get_float( window->client, UCVIEW_GCONF_DIR "/video_frame_rate", NULL );
	 if( fps <= 0.0f )
	 {
	    fps = 30.0f;
	 }

	 downsize = gconf_client_get_int( window->client, UCVIEW_GCONF_DIR "/video_downsize", NULL );
	 if( downsize < 1 )
	 {
	    downsize = 1;
	 }

	 g_object_get( window->display, "backend_fourcc", &backend_fourcc, NULL );
      
	 window->video_object = ucil_create_video_file( filename, &window->format, "ogg/theora", 
							"fill_frames", window->time_lapse ? 0 : 1, 
							"bitrate", bitrate, 
							"quality", quality, 
							"encode_frame_cb", record_encode_frame_cb, window, 
							"audio", 
							gconf_client_get_bool( window->client, UCVIEW_GCONF_DIR "/record_audio", NULL ), 
							"nocopy", backend_fourcc == window->format.fourcc, 
							"fps", fps, 
							"async_audio_encoding",
							gconf_client_get_bool( window->client, UCVIEW_GCONF_DIR "/async_audio_encode", NULL ), 
							"downsize", downsize, 
							NULL );
      }
      else if( !strcmp( codec, "avi/raw" ) )
      {
	 guint fourcc;

	 fourcc = (guint)gconf_client_get_int( window->client, UCVIEW_GCONF_DIR "/rawavi_colorformat", NULL );
	 if( !fourcc )
	 {
	    fourcc = window->format.fourcc;
	 }
	 
	 window->video_object = ucil_create_video_file( filename, &window->format, "avi/raw", "fourcc", fourcc, NULL );
      }
      else
      {
	 window->video_object = NULL;
/* 	 error_string = "Unsupported video codec"; */
      }
	 

      if( window->video_object )
      {
	 window->record = TRUE;
	 if( window->last_video_file )
	 {
	    g_free( window->last_video_file );
	 }
	 window->last_video_file = filename;
      }
      else
      {
	 GtkWidget *info_box;

	 info_box = g_object_new( UCVIEW_INFO_BOX_TYPE, 
				  "image", gtk_image_new_from_stock( GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_DIALOG ), 
				  "text", gtk_label_new( _("Failed to start video recording!") ), 
				  NULL );
	 ucview_info_box_add_action_widget( UCVIEW_INFO_BOX( info_box ), gtk_button_new_from_stock( GTK_STOCK_CLOSE ), GTK_RESPONSE_CLOSE );
	 g_signal_connect( info_box, "response", (GCallback)gtk_widget_destroy, NULL );
	 ucview_window_set_info_box( window, info_box );

	 gtk_toggle_action_set_active( GTK_TOGGLE_ACTION( action ), FALSE );
      }
      
   }
   else
   {
      window->record = FALSE;
      if( window->video_object )
      {
	 ucil_close_video_file( window->video_object );
	 if( gconf_client_get_bool( window->client, UCVIEW_GCONF_DIR "/record_audio", NULL ) &&  
	     gconf_client_get_bool( window->client, UCVIEW_GCONF_DIR "/async_audio_encode", NULL ) )
	 {
	    GtkWidget *vbox;
	    GtkWidget *info_box;
	    GtkAction *record_video_action;

	    record_video_action = gtk_ui_manager_get_action( ucview_window_get_ui_manager( UCVIEW_WINDOW( window ) ), "/MenuBar/EditMenu/RecordVideo" );
	    if( record_video_action )
	    {
	       gtk_action_set_sensitive( GTK_ACTION( record_video_action ), FALSE );
	    }
	    vbox = gtk_vbox_new( FALSE, 6 );
	    gtk_box_pack_start( GTK_BOX( vbox ), gtk_label_new( _("Processing video file" ) ), FALSE, TRUE, 0 );
	    window->video_processing_progressbar = gtk_progress_bar_new();
	    gtk_box_pack_start( GTK_BOX( vbox ), window->video_processing_progressbar, FALSE, TRUE, 0 );

	    info_box = g_object_new( UCVIEW_INFO_BOX_TYPE, "image", gtk_image_new_from_stock( GTK_STOCK_DIALOG_INFO, GTK_ICON_SIZE_DIALOG ),
				     "text", vbox, NULL );
	    ucview_window_set_info_box( window, info_box );
	    window->video_processing_fract = 0.0;
	    gtk_widget_show_all( info_box );
	    run_worker( (void *(*)(void *))process_video_worker, window );
	    gtk_widget_destroy( info_box );
	    window->video_processing_progressbar = NULL;
	    if( record_video_action )
	    {
	       gtk_action_set_sensitive( GTK_ACTION( record_video_action ), TRUE );
	    }
	 }
	    
	 g_signal_emit( G_OBJECT( window ), ucview_signals[ UCVIEW_VIDEO_FILE_CREATED_SIGNAL ], 0, window->last_video_file );

	 if( !gconf_client_get_bool( window->client, UCVIEW_GCONF_DIR "/hide_video_saved_box", NULL ) )
	 {
	    GtkWidget *vbox;
	    GtkWidget *info_text;
	    GtkWidget *info_box;
	    GtkWidget *check_button;
	    
	    gchar *message;
	  
	    vbox = gtk_vbox_new( FALSE, 6 );

	    message = g_strdup_printf( _("Video saved to: \n<i>%s</i>"), window->last_video_file );
	 
	    info_text = gtk_label_new( message );
	    g_free( message );
	    gtk_misc_set_alignment( GTK_MISC( info_text ), 0.0f, 0.0f );
	    g_object_set( info_text, "use-markup", TRUE, NULL );
	    
	    gtk_box_pack_start( GTK_BOX( vbox ), info_text, FALSE, TRUE, 0 );

	    check_button = gtk_check_button_new_with_label( _("Do not show this message again") );
	    g_signal_connect( check_button, "toggled", (GCallback)hide_video_info_box_toggled_cb, window );
	    gtk_box_pack_start( GTK_BOX( vbox ), check_button, FALSE, TRUE, 0 );
	       
	    info_box = g_object_new( UCVIEW_INFO_BOX_TYPE, "image", gtk_image_new_from_stock( GTK_STOCK_DIALOG_INFO, GTK_ICON_SIZE_DIALOG ),
				     "text", vbox, NULL );
	       
	    ucview_info_box_add_action_widget( UCVIEW_INFO_BOX( info_box ),
					       gtk_button_new_from_stock( GTK_STOCK_CLOSE ),
					       GTK_RESPONSE_CLOSE );
	       
	    g_signal_connect( info_box, "response", G_CALLBACK( gtk_widget_destroy ), NULL );
	    gtk_widget_show_all( info_box );
	       
	    ucview_window_set_info_box( window, info_box );   
	 }
      }
   }      
}

void play_toggled_cb( GtkAction *action, UCViewWindow *window )
{
   if( gtk_toggle_action_get_active( GTK_TOGGLE_ACTION( action ) ) )
   {
      GtkAction *record_video_action;
      
      record_video_action = gtk_ui_manager_get_action( ucview_window_get_ui_manager( UCVIEW_WINDOW( window ) ), "/MenuBar/EditMenu/RecordVideo" );
      g_assert( record_video_action );
      gtk_toggle_action_set_active( GTK_TOGGLE_ACTION( record_video_action ), FALSE );

      if( window->last_video_file )
      {
	 if( SUCCESS( ucil_open_video_file( &window->video_file_handle, window->last_video_file ) ) )
	 {
	    if( SUCCESS( unicapgtk_video_display_set_handle( UNICAPGTK_VIDEO_DISPLAY( window->display ), window->video_file_handle ) ) )
	    {
	       unicapgtk_video_display_start( UNICAPGTK_VIDEO_DISPLAY( window->display ) );
	    }
	 }
      }
   }
   else
   {
      unicapgtk_video_display_set_handle( UNICAPGTK_VIDEO_DISPLAY( window->display ), window->device_handle );
      unicapgtk_video_display_start( UNICAPGTK_VIDEO_DISPLAY( window->display ) );
   }
}

void clipboard_copy_image_cb( GtkAction *action, UCViewWindow *window )
{
   window->clipboard_copy_image = TRUE;
   window->still_image = TRUE;
   g_message( "copy cb" );
}

void save_still_image_cb( GtkAction *action, UCViewWindow *window )
{
   ucview_window_save_still_image( window, NULL, NULL, NULL );
}

void device_dialog_ok_clicked_cb( GtkButton *button, UCViewDeviceDialog *dialog )
{
   UCViewWindow *window;
   unicap_handle_t handle;   
   
   window = g_object_get_data( G_OBJECT(dialog), "ucview_window" );
   g_assert( window );

   handle = ucview_device_dialog_get_handle( dialog );

   if( handle ){
      unicap_device_t current_device;
      unicap_device_t selected_device;
      unicap_format_t format;
      ucview_device_dialog_get_format( dialog, &format );
      
      unicap_get_device (window->device_handle, &current_device);
      unicap_get_device (handle, &selected_device);
      
      if (!strcmp (current_device.identifier, selected_device.identifier)){
	 ucview_window_stop_live (window);
	 ucview_window_set_video_format( window, &format );
	 ucview_window_start_live (window);
	 unicap_close (handle);
      } else {
	 ucview_window_set_handle( window, handle );
	 ucview_window_set_video_format( window, &format );
	 ucview_window_start_live( window );
	 ucview_window_clear_info_box( window );
      }
   }

   gtk_widget_hide( GTK_WIDGET(dialog) );
}

void change_device_cb( GtkAction *action, UCViewWindow *window )
{
	gtk_widget_show (ucview_window_get_device_dialog (window));
   /* GtkWidget *device_dialog; */
   /* unicap_device_t device; */
   
   /* unicap_get_device( window->device_handle, &device ); */
   
   /* device_dialog = g_object_new( UCVIEW_DEVICE_DIALOG_TYPE, "restore-device", FALSE, NULL ); */
   /* ucview_device_dialog_select_device( UCVIEW_DEVICE_DIALOG( device_dialog), &device ); */
   /* gtk_widget_show_all( device_dialog ); */

   /* g_object_set_data( G_OBJECT(device_dialog), "ucview_window", window ); */
   /* g_signal_connect( UCVIEW_DEVICE_DIALOG(device_dialog)->ok_button, "clicked", (GCallback)device_dialog_ok_clicked_cb, device_dialog ); */
   /* g_signal_connect( device_dialog, "delete-event", (GCallback)gtk_widget_destroy, NULL ); */
}

void pause_state_toggled_cb( GtkAction *action, UCViewWindow *window )
{
   unicapgtk_video_display_set_pause( UNICAPGTK_VIDEO_DISPLAY( window->display ), 
				      gtk_toggle_action_get_active( GTK_TOGGLE_ACTION( action ) ) );
}

void display_property_dialog_cb( GtkAction *action, UCViewWindow *window )
{
   gtk_window_present( GTK_WINDOW( window->property_dialog ) );
}

void display_settings_dialog_cb( GtkAction *action, UCViewWindow *window )
{
   gtk_window_present( GTK_WINDOW( window->settings_dialog ) );
}
