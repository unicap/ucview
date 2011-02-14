/*
** ucview.c
** 
** Made by (Arne Caspari)
** Login   <arne@arne-laptop>
** 
*/

/*
  Copyright (C) 2007-2008  Arne Caspari <arne@unicap-imaging.org>

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

/*
  Main application window and mainloop / callback handling
 */

#include "config.h"

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


#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <signal.h>

#include "ucview.h"
#include "ucview-window.h"
#include "settings_dialog.h"
#include "ucview-info-box.h"
#include "checkperm.h"
#include "callbacks.h"
#include "gui.h"
#include "user_config.h"
#include "plugin_loader.h"
#include "preferences.h"
#include "worker.h"

#include "icon-ucview.h"
#include "icons.h"
#include "sidebar.h"

#include "marshal.h"




enum
{
   PROP_0 = 0, 
   PROP_UNICAP_HANDLE, 
};



static void ucview_window_class_init( UCViewWindowClass *klass );
static void ucview_window_init( UCViewWindow *ucv );
static void ucview_window_set_property( GObject *obj, guint property_id, const GValue *value, GParamSpec *pspec );
static void ucview_window_get_property( GObject *obj, guint property_id, GValue *value, GParamSpec *pspec );
static void ucview_window_destroy( GtkObject *obj );
static void ucview_window_realize( UCViewWindow *window );
static void ucview_window_unrealize( UCViewWindow *window );


static void build_main_window( UCViewWindow *window );
static gboolean resize_window( UCViewWindow *ucv );
static void new_frame_cb( unicap_event_t event, unicap_handle_t handle, unicap_data_buffer_t *buffer, UCViewWindow *window );
static void device_removed_cb( unicap_event_t event, unicap_handle_t handle, UCViewWindow *window );
static void predisplay_cb( UnicapgtkVideoDisplay *ugtk, unicap_data_buffer_t *buffer, UCViewWindow *window );
static gboolean default_save_handler( unicap_data_buffer_t *buffer, gchar *path, gchar *filetype, UCViewWindow *window, GError **err );


#define UCVIEW_WINDOW_GET_PRIVATE( obj ) ( G_TYPE_INSTANCE_GET_PRIVATE( ( obj ), UCVIEW_WINDOW_TYPE, UCViewWindowPrivate ) )


G_DEFINE_TYPE( UCViewWindow, ucview_window, GTK_TYPE_WINDOW );

guint ucview_signals[ UCVIEW_LAST_SIGNAL ] = { 0 };


typedef struct _CrashData CrashData;

struct _CrashData
{
      GConfClient *client;
      UCViewPluginData *plugin;
};

CrashData crash_data;

typedef struct _ImageFileTypeDetail ImageFileTypeDetail;

struct _ImageFileTypeDetail
{
   gchar *filetype;
   gchar *suffix;
   UCViewSaveImageFileHandler handler;
   gpointer user_ptr;
};

   


#define FPS_VALUES 30


struct _UCViewWindowPrivate
{
	GtkUIManager *ui;
	GtkWidget *statusbar;
	GtkWidget *device_dialog;
	gdouble fps_times[FPS_VALUES];
	GTimer *fps_timer;
	gdouble fps;
	gint fps_timeout;
	
	GList *image_file_types;
};



static void crash_handler( int signal, siginfo_t *info, void *_data )
{
   gchar *gconf_path;
   
   if( crash_data.plugin )
   {
      gchar *keyname = g_path_get_basename( g_module_name( crash_data.plugin->module ) );
   
      gconf_path = g_build_path( "/", UCVIEW_GCONF_DIR, "plugins", keyname, NULL );
      gconf_client_set_bool( crash_data.client, gconf_path, FALSE, NULL );
      gconf_client_set_string( crash_data.client, UCVIEW_GCONF_DIR "/crash_module", g_module_name( crash_data.plugin->module ), NULL );
      g_free( gconf_path );
   }
   else
   {
      gconf_client_unset( crash_data.client, UCVIEW_GCONF_DIR "/crash_module", NULL );
   }
   
   gconf_client_set_bool( crash_data.client, UCVIEW_GCONF_DIR "/crash_detected", TRUE, NULL );
};


static void ucview_window_class_init( UCViewWindowClass *klass )
{
   GConfClient *client;
   struct sigaction sigact;
   GObjectClass *object_class = G_OBJECT_CLASS( klass );
   GtkObjectClass *gtk_object_class = GTK_OBJECT_CLASS( klass );

   g_type_class_add_private( object_class, sizeof( UCViewWindowPrivate ) );
   
   object_class->set_property = ucview_window_set_property;
   object_class->get_property = ucview_window_get_property;
   gtk_object_class->destroy = ucview_window_destroy;
   
   g_object_class_install_property( object_class, 
				    PROP_UNICAP_HANDLE, 
				    g_param_spec_pointer( "unicap-handle", NULL, NULL, 
							  G_PARAM_READABLE | G_PARAM_WRITABLE /* | G_PARAM_CONSTRUCT_ONLY */ ) );

   client = gconf_client_get_default();
   gconf_client_add_dir( client, UCVIEW_GCONF_DIR, GCONF_CLIENT_PRELOAD_NONE, NULL );
   klass->gconf_client = client;

   ucview_signals[ UCVIEW_DISPLAY_IMAGE_SIGNAL ] = 
      g_signal_new( "display_image", 
		    G_TYPE_FROM_CLASS( klass ), 
		    G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION, 
		    G_STRUCT_OFFSET( UCViewWindowClass, ucview_window ), 
		    NULL, 
		    NULL, 
		    g_cclosure_marshal_VOID__POINTER, 
		    G_TYPE_NONE, 
		    1, 
		    G_TYPE_POINTER );
   ucview_signals[ UCVIEW_SAVE_IMAGE_SIGNAL ] = 
      g_signal_new( "save_image", 
		    G_TYPE_FROM_CLASS( klass ), 
		    G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION, 
		    G_STRUCT_OFFSET( UCViewWindowClass, ucview_window ), 
		    NULL, 
		    NULL, 
		    g_cclosure_user_marshal_VOID__POINTER_STRING, 
		    G_TYPE_NONE, 
		    2, 
		    G_TYPE_POINTER, 
		    G_TYPE_STRING );
   ucview_signals[ UCVIEW_IMAGE_FILE_CREATED_SIGNAL ] = 
      g_signal_new( "image_file_created", 
		    G_TYPE_FROM_CLASS( klass ), 
		    G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION, 
		    G_STRUCT_OFFSET( UCViewWindowClass, ucview_window ), 
		    NULL, NULL, 
		    g_cclosure_marshal_VOID__STRING, 
		    G_TYPE_NONE, 
		    1, 
		    G_TYPE_STRING );
   ucview_signals[ UCVIEW_RECORD_START_SIGNAL ] = 
      g_signal_new( "record_start", 
		    G_TYPE_FROM_CLASS( klass ), 
		    G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION, 
		    G_STRUCT_OFFSET( UCViewWindowClass, ucview_window ), 
		    NULL, NULL, 
		    g_cclosure_user_marshal_VOID__STRING_STRING, 
		    G_TYPE_NONE, 
		    2, 
		    G_TYPE_STRING, 
		    G_TYPE_STRING );
   ucview_signals[ UCVIEW_TIME_LAPSE_START_SIGNAL ] = 
      g_signal_new( "time_lapse_start", 
		    G_TYPE_FROM_CLASS( klass ), 
		    G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION, 
		    G_STRUCT_OFFSET( UCViewWindowClass, ucview_window ), 
		    NULL, NULL, 
		    g_cclosure_marshal_VOID__VOID, 
		    G_TYPE_NONE, 0 );
      
   ucview_signals[ UCVIEW_VIDEO_FILE_CREATED_SIGNAL ] = 
      g_signal_new( "video_file_created", 
		    G_TYPE_FROM_CLASS( klass ), 
		    G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION, 
		    G_STRUCT_OFFSET( UCViewWindowClass, ucview_window ), 
		    NULL, NULL, 
		    g_cclosure_marshal_VOID__STRING, 
		    G_TYPE_NONE, 
		    1, 
		    G_TYPE_STRING );

   ucview_signals[ UCVIEW_VIDEO_FORMAT_CHANGED_SIGNAL ] = 
      g_signal_new( "video_format_changed", 
		    G_TYPE_FROM_CLASS( klass ), 
		    G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION, 
		    G_STRUCT_OFFSET( UCViewWindowClass, ucview_window ), 
		    NULL, NULL, 
		    g_cclosure_marshal_VOID__POINTER, 
		    G_TYPE_NONE, 
		    1, 
		    G_TYPE_POINTER );

   ucview_signals[ UCVIEW_IMAGE_FILE_TYPE_ADDED_SIGNAL ] = 
      g_signal_new( "image_file_type_added", 
		    G_TYPE_FROM_CLASS( klass ), 
		    G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION, 
		    G_STRUCT_OFFSET( UCViewWindowClass, ucview_window ), 
		    NULL, NULL, 
		    g_cclosure_marshal_VOID__POINTER, 
		    G_TYPE_NONE, 
		    1, 
		    G_TYPE_STRING );

   ucview_signals[ UCVIEW_IMAGE_FILE_TYPE_REMOVED_SIGNAL ] = 
      g_signal_new( "image_file_type_removed", 
		    G_TYPE_FROM_CLASS( klass ), 
		    G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION, 
		    G_STRUCT_OFFSET( UCViewWindowClass, ucview_window ), 
		    NULL, NULL, 
		    g_cclosure_marshal_VOID__POINTER, 
		    G_TYPE_NONE, 
		    1, 
		    G_TYPE_STRING );
		    

   crash_data.client = client;
   crash_data.plugin = NULL;
   
   sigact.sa_sigaction = crash_handler;
   sigact.sa_flags = SA_RESETHAND | SA_SIGINFO;
   sigaction( SIGSEGV, &sigact, NULL );
}

static void crash_close_cb( GtkWidget *box, gint response, UCViewWindow *window )
{
   gconf_client_set_bool( crash_data.client, UCVIEW_GCONF_DIR "/crash_detected", FALSE, NULL );
   gtk_widget_destroy( box );
}


static gboolean delete_window_cb( GtkWidget *widget, GdkEvent * event, gpointer data )
{
   prefs_store_window_state( UCVIEW_WINDOW( widget ) );

   gtk_main_quit();
   
   return TRUE;
}


static void ucview_window_init( UCViewWindow *window )
{
   UCViewWindowClass *klass = UCVIEW_WINDOW_GET_CLASS( window );
   GtkWidget *vbox;
   gint w;

   window->priv = UCVIEW_WINDOW_GET_PRIVATE( window );
   memset( window->priv->fps_times, 0x0, sizeof( window->priv->fps ) );

   window->modules = NULL;
   window->mutex = g_mutex_new();
   window->record = FALSE;
   window->still_image = FALSE;
   window->last_video_file = NULL;
   window->property_dialog = NULL;
   window->display = NULL;
   window->client = klass->gconf_client;
   window->glade = glade_xml_new( INSTALL_PREFIX "/share/ucview/ucview.glade", "ucview_main_hbox", NULL );
   window->priv->fps_timeout = 0;
   window->priv->image_file_types = NULL;
   memset( &window->still_image_buffer, 0x0, sizeof( unicap_data_buffer_t ) );

   g_signal_connect( window, "realize", (GCallback)ucview_window_realize, NULL );
   g_signal_connect( window, "unrealize", (GCallback)ucview_window_unrealize, NULL );
   
   if( !window->glade )
   {
      window->glade = glade_xml_new( "ucview.glade", "ucview_main_hbox", NULL );
   }
   
   window->video_filename = NULL;
   build_main_window( window );

   window->size_restored = prefs_restore_window_state( window );

   window->priv->fps_timer = g_timer_new();


   g_signal_connect( window, "delete-event", (GCallback)delete_window_cb, NULL );

   window->gconf_cnxn_id = gconf_client_notify_add( window->client, UCVIEW_GCONF_DIR, 
						    (GConfClientNotifyFunc) gconf_notification_cb, window, NULL, NULL );

   gconf_client_notify( window->client, UCVIEW_GCONF_DIR "/show_fps" );
   gconf_client_notify( window->client, UCVIEW_GCONF_DIR "/scale_to_fit" );

   if( gconf_client_get_bool(  window->client, UCVIEW_GCONF_DIR "/crash_detected", NULL ) )
   {
      GtkWidget *info_box;
      GtkWidget *info_text;
      gchar *message;
      gchar *modname;		
   
      modname = gconf_client_get_string( window->client, UCVIEW_GCONF_DIR "/crash_module", NULL );

      if( modname )
      {
	 message = g_strdup_printf( _("<b>Crash detected!</b>\nUCView detected a malfunction of the \n<i>'%s'</i>\nplugin. The plugin got disabled." ), 
				     modname );
	 g_free( modname );
      }
      else
      {
	 message = g_strdup( _("<b>Crash detected!</b>\nUCView detected a crash but it could not find the cause. \nPlease report a bug." ) );
      }

      info_text = gtk_label_new( message );
      gtk_misc_set_alignment( GTK_MISC( info_text ), 0.0f, 0.0f );
      g_free( message );
      
      g_object_set( info_text, "use-markup", TRUE, NULL );
      info_box = g_object_new( UCVIEW_INFO_BOX_TYPE, "image", gtk_image_new_from_stock( GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_DIALOG ),
			       "text", info_text, NULL );
      ucview_info_box_add_action_widget( UCVIEW_INFO_BOX( info_box ),
					 gtk_button_new_from_stock( GTK_STOCK_CLOSE ),
					 GTK_RESPONSE_CLOSE );
      g_signal_connect( info_box, "response", G_CALLBACK( crash_close_cb ), window );
      
      ucview_window_set_info_box( window, info_box );
   }

   window->display = unicapgtk_video_display_new();/* create_display_widget( handle ); */
   window->display_ebox = gtk_event_box_new();
   gtk_container_add( GTK_CONTAINER( window->display_ebox ), window->display );
   gtk_scrolled_window_add_with_viewport( GTK_SCROLLED_WINDOW( window->display_window ), window->display_ebox );
   g_object_set( window->display, "scale-to-fit", gconf_client_get_bool( window->client, UCVIEW_GCONF_DIR "/scale_to_fit", NULL ), NULL );

   vbox = glade_xml_get_widget( window->glade, "ucview_main_vbox" );
   gtk_widget_show_all( GTK_WIDGET( vbox ) );

   /*
     disable_xv configuration parameter is deprecated but still supported. Use 'backend' parameter instead
    */
   if( gconf_client_get_bool( window->client, UCVIEW_GCONF_DIR "/disable_xv", NULL ) )
   {
      g_object_set( window->display, "backend", "gtk", NULL );
   }else{
      gchar *backend;
      backend = gconf_client_get_string( window->client, UCVIEW_GCONF_DIR "/backend", NULL );
      if( backend ){
	 g_object_set( window->display, "backend", backend, NULL );
	 g_free( backend );
      }
   }

   window->sidebar = sidebar_new( window );
   gtk_box_pack_start( GTK_BOX( glade_xml_get_widget( window->glade, "ucview_sidebar_box" ) ), 
		       window->sidebar, 
		       TRUE, TRUE, 0 );

   w = gconf_client_get_int( window->client, UCVIEW_GCONF_DIR "/sidebar_width", NULL );
   if( !w )
   {
      w = gconf_client_get_int( window->client, UCVIEW_GCONF_DIR "/sidebar_image_width", NULL );
      if( w ){
	 w += 8;
      }else{
	 w = 136;
      }
   }
   
   gtk_paned_set_position( GTK_PANED( glade_xml_get_widget( window->glade, "ucview_hpaned" ) ), w );

   ucview_window_set_fullscreen( window, gconf_client_get_bool( window->client, UCVIEW_GCONF_DIR "/fullscreen", NULL ) );

   g_signal_connect( G_OBJECT( window->display ), "unicapgtk_video_display_predisplay", G_CALLBACK( predisplay_cb ), window );

   ucview_load_plugins( window );
   settings_dialog_update_plugins( SETTINGS_DIALOG( window->settings_dialog ), window );

   ucview_enable_all_plugins( window );   
}

static void ucview_window_set_property( GObject *object, 
					guint property_id, 
					const GValue *value, 
					GParamSpec *pspec )
{
   UCViewWindow *window = UCVIEW_WINDOW( object );
   
   switch( property_id )
   {
      case PROP_UNICAP_HANDLE:
      {
	 unicap_handle_t handle = NULL;	 
	 handle = ( unicap_handle_t ) g_value_get_pointer( value );

	 ucview_window_set_handle( window, handle );
      }
      break;

      default:
	 G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, pspec );
	 break;
   }
}
      
static void ucview_window_get_property( GObject *object, 
					guint property_id, 
					GValue *value, 
					GParamSpec *pspec )
{
   UCViewWindow *window = UCVIEW_WINDOW( object );
   
   switch( property_id )
   {
      case PROP_UNICAP_HANDLE:
      {
	 g_value_set_pointer( value, window->device_handle );
      }
      break;

      default:
	 G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, pspec );
	 break;
   }
}


static void ucview_window_realize( UCViewWindow *window )
{
}

static void ucview_window_unrealize( UCViewWindow *window )
{
/*    ucview_unload_plugins( window ); */
}

static gboolean property_dialog_filter( unicap_property_t *property, UCViewWindow *window )
{
   GConfClient *client;
   GSList *list;
   gboolean filter = FALSE;
   
   client = gconf_client_get_default( );

   list = gconf_client_get_list( client, UCVIEW_GCONF_DIR "/property_filter", GCONF_VALUE_STRING, NULL );
   
   if( g_slist_find( list, property->identifier ) != NULL )
   {
      filter = TRUE;
   }
   
   g_slist_free( list );

   g_object_unref( client );
   
   return filter;
}

void ucview_window_get_video_format( UCViewWindow *window, unicap_format_t *format )
{
   unicap_copy_format( format, &window->format );
}

unicap_status_t ucview_window_set_video_format( UCViewWindow *window, unicap_format_t *format )
{
   unicap_status_t status = STATUS_FAILURE;
   format->buffer_type = UNICAP_BUFFER_TYPE_SYSTEM;
   status = unicapgtk_video_display_set_format( UNICAPGTK_VIDEO_DISPLAY( window->display ), format );
   if( SUCCESS( status ) ){
      unicapgtk_video_display_set_new_frame_callback( UNICAPGTK_VIDEO_DISPLAY( window->display ), 
						      UNICAPGTK_CALLBACK_FLAGS_BEFORE, 
						      (unicap_new_frame_callback_t)new_frame_cb, 
						      window );
   } else {
      // failed! Start again with any format
      g_object_set( window->display, "backend_fourcc", 0, NULL );
      status = unicapgtk_video_display_set_format( UNICAPGTK_VIDEO_DISPLAY( window->display ), format );
      unicapgtk_video_display_set_new_frame_callback( UNICAPGTK_VIDEO_DISPLAY( window->display ), 
						      UNICAPGTK_CALLBACK_FLAGS_BEFORE, 
						      (unicap_new_frame_callback_t)new_frame_cb, 
						      window );
   }

   if( window->still_image_buffer.data ){
      free( window->still_image_buffer.data );
   }

   unicap_copy_format( &window->format, format );
   unicap_copy_format( &window->still_image_buffer.format, format );
   window->still_image_buffer.format.fourcc = UCIL_FOURCC( 'R', 'G', 'B', '3' );
   window->still_image_buffer.format.bpp = 24;
   window->still_image_buffer.format.buffer_size = window->still_image_buffer.format.size.width * 
      window->still_image_buffer.format.size.height * 3;
   window->still_image_buffer.buffer_size = window->still_image_buffer.format.buffer_size;
   window->still_image_buffer.data = malloc( window->still_image_buffer.format.buffer_size );

   g_signal_emit( G_OBJECT( window ), ucview_signals[ UCVIEW_VIDEO_FORMAT_CHANGED_SIGNAL ], 0, format  );


   return status;
}


void ucview_window_set_handle( UCViewWindow *window, unicap_handle_t handle )
{
   GtkWidget *property_dialog;
   unicap_format_t format;
   unicap_device_t device;
   GString *title;

   if( window->device_handle ){
      unicapgtk_video_display_stop( UNICAPGTK_VIDEO_DISPLAY( window->display ) );
      unicap_close( window->device_handle );
   }
   window->device_handle = handle;

   unicap_get_device( handle, &device );
   title = g_string_new( "" );
   g_string_printf( title, "%s - UCView", device.identifier );
   gtk_window_set_title( GTK_WINDOW( window ), title->str );
   g_string_free( title, TRUE );

   unicap_register_callback( handle, UNICAP_EVENT_DEVICE_REMOVED, (unicap_callback_t )device_removed_cb, window );
   
   unicap_get_format( handle, &format );
   if( !window->size_restored )
   {
      if( ( format.size.width > 1024 ) || ( format.size.height > 768 ) )
      {
	 gtk_widget_set_size_request( window->display_window, 1024, 768 );
      }
      else
      {
	 gtk_widget_set_size_request( window->display_window, format.size.width + 10, format.size.height + 10 );
      }
   }

   unicapgtk_video_display_set_handle( UNICAPGTK_VIDEO_DISPLAY( window->display ), handle );
   
   /*
     Use of encoder_fastpath parameter is deprecated. Use backend_fourcc parameter instead. 
    */
   if( gconf_client_get_bool( window->client, UCVIEW_GCONF_DIR "/encoder_fastpath", NULL ) )
   {
      g_object_set( window->display, "backend_fourcc", UCIL_FOURCC( 'I', '4', '2', '0' ), NULL );
   } else {
      gchar *fourcc_str;
      fourcc_str = gconf_client_get_string( window->client, UCVIEW_GCONF_DIR "/backend_fourcc", NULL );
      if( fourcc_str && strlen( fourcc_str ) == 4 ){
	 guint fourcc = ( (guint)fourcc_str[0] << 24 ) | ( (guint)fourcc_str[1] << 16 ) | ( (guint)fourcc_str[2] << 8 ) | ( fourcc_str[3] );
	 g_object_set( window->display, "backend_fourcc", fourcc, NULL );
      }
   }      

   if( window->property_dialog )
   {
      gtk_widget_destroy( window->property_dialog );
   }
   property_dialog = unicapgtk_property_dialog_new_by_handle( handle );

   unicapgtk_property_dialog_set_filter( UNICAPGTK_PROPERTY_DIALOG( property_dialog ), 
					 (unicap_property_filter_func_t) property_dialog_filter, window );
   g_object_set( property_dialog, "update-interval", 
		 gconf_client_get_int( window->client, UCVIEW_GCONF_DIR "/property_update_interval", NULL ) , NULL );
   g_signal_connect_swapped( G_OBJECT( window ), "destroy", G_CALLBACK( gtk_widget_destroy ), property_dialog );
   g_signal_connect( G_OBJECT( property_dialog ), "delete-event", G_CALLBACK( gtk_widget_hide_on_delete ), NULL );
   gtk_window_set_transient_for( GTK_WINDOW( property_dialog ), GTK_WINDOW( window ) );
   window->property_dialog = property_dialog;
   gtk_widget_hide( window->property_dialog );

   g_idle_add( (GSourceFunc)resize_window , window );


   unicapgtk_property_dialog_reset( UNICAPGTK_PROPERTY_DIALOG( property_dialog ) );
}

unicap_status_t ucview_window_start_live( UCViewWindow *window )
{
   return unicapgtk_video_display_start( UNICAPGTK_VIDEO_DISPLAY( window->display ) );
}

void ucview_window_stop_live( UCViewWindow *window )
{
   unicapgtk_video_display_stop( UNICAPGTK_VIDEO_DISPLAY( window->display ) );
}

static ImageFileTypeDetail *get_image_file_type_detail( UCViewWindow *window, gchar *filetype )
{
   GList *entry;
   for( entry = g_list_first( window->priv->image_file_types ); entry; entry = g_list_next( entry ) ){
      ImageFileTypeDetail *detail = entry->data;
      if( !strcmp( filetype, detail->filetype ) ){
	 return detail;
      }
   }
   
   return NULL;
}

void ucview_window_register_image_file_type( UCViewWindow *window, gchar *filetype, gchar *suffix, UCViewSaveImageFileHandler handler, gpointer user_ptr )
{
   ImageFileTypeDetail *detail = g_new0( ImageFileTypeDetail, 1 );
   

   detail->filetype = g_strdup( filetype );
   detail->suffix = g_strdup( suffix );
   detail->handler = handler;
   detail->user_ptr = user_ptr;

   g_mutex_lock( window->mutex );
   window->priv->image_file_types = g_list_append( window->priv->image_file_types, detail );
   g_mutex_unlock( window->mutex );
   
   g_signal_emit( G_OBJECT( window ), ucview_signals[ UCVIEW_IMAGE_FILE_TYPE_ADDED_SIGNAL ], 0, filetype  );
}

void ucview_window_deregister_image_file_type( UCViewWindow *window, gchar *filetype )
{
   GList *entry;

   for( entry = g_list_first( window->priv->image_file_types ); entry; entry = g_list_next( entry ) ){
      ImageFileTypeDetail *detail = ( ImageFileTypeDetail* )entry->data;
      if( !strcmp( detail->filetype, filetype ) ){
	 window->priv->image_file_types = g_list_remove( window->priv->image_file_types, detail );
	 g_free( detail->filetype );
	 g_free( detail->suffix );
	 g_free( detail );
	 g_signal_emit( G_OBJECT( window ), ucview_signals[ UCVIEW_IMAGE_FILE_TYPE_REMOVED_SIGNAL ], 0, filetype  );
	 break;
      }
   }
}




gboolean ucview_window_save_still_image( UCViewWindow *window, const gchar *_filename, const gchar *_filetype, GError **error )
{
   gchar *path = NULL;
   gchar *filename = g_strdup( _filename );
   gchar *filetype = g_strdup( _filetype );
   unicap_data_buffer_t *buffer;

   unicapgtk_video_display_get_data_buffer( UNICAPGTK_VIDEO_DISPLAY( window->display ), 
					    &buffer );
   
   
   if( !filename ){
      path = gconf_client_get_string( window->client, UCVIEW_GCONF_DIR "/still_image_file_path", NULL );
      if( !g_file_test( path, ( G_FILE_TEST_IS_DIR ) ) ){
	 g_set_error( error, UCVIEW_ERROR, 0, _("Path '%s' does not exist"), path );
	 g_free( path );
	 return FALSE;
      }
   }
   
   if( !filetype ){
      filetype = gconf_client_get_string( window->client, UCVIEW_GCONF_DIR "/still_image_file_type", NULL );
      if( !filetype ){
	 filetype = g_strdup( "jpeg" );
      }
   }
   
   ImageFileTypeDetail *detail = get_image_file_type_detail( window, filetype );
   if( !detail ){
      g_set_error( error, UCVIEW_ERROR, 0, _("Unknown image format") );
      return FALSE;
   }
   
   if( !filename ){
      int i;
      for( i = 0; i < 99999; i++ ){
	 gchar tmp[128];
	 sprintf( tmp, "%s%cImage_%05d.%s", path, G_DIR_SEPARATOR, i, detail->suffix );
	 g_free( filename );
	 filename = g_strdup( tmp );
	 if( !g_file_test( filename, G_FILE_TEST_EXISTS ) )
	    break;
      }
   }
   
/*    strcpy( window->still_image_path, filename ); */
/*    strcpy( window->still_image_file_type, filetype ); */
/*    window->still_image = TRUE; */
   g_signal_emit( G_OBJECT( window ), ucview_signals[ UCVIEW_SAVE_IMAGE_SIGNAL ], 0, buffer, filename );
   if( detail->handler( buffer, filename, filetype, detail->user_ptr, NULL ) )
      g_signal_emit( G_OBJECT( window ), ucview_signals[ UCVIEW_IMAGE_FILE_CREATED_SIGNAL ], 0, filename );

   sidebar_add_image( SIDEBAR( window->sidebar ), buffer, filename );
   
   g_free( path );
   g_free( filetype );
   g_free( filename );

   return TRUE;
}



static void ucview_window_destroy( GtkObject *obj )
{
   UCViewWindow *ucv = UCVIEW_WINDOW( obj );   

   gdk_threads_leave();
   if( ucv->device_handle )
   {
      if( ucv->display )
      {
	 unicapgtk_video_display_stop( UNICAPGTK_VIDEO_DISPLAY( ucv->display ) );
      }
      user_config_store_device( ucv->device_handle );
      unicap_close( ucv->device_handle );
   }
   gdk_threads_enter();
 

   ucview_unload_plugins( ucv );

   if( ucv->priv->fps_timeout != 0 )
   {
      g_source_remove( ucv->priv->fps_timeout );
      ucv->priv->fps_timeout = 0;
   }

/*    gtk_container_foreach( GTK_CONTAINER( obj ), (GtkCallback) gtk_widget_destroy, NULL ); */

   gconf_client_notify_remove( ucv->client, ucv->gconf_cnxn_id );

}

void ucview_window_clear_info_box( UCViewWindow *window )
{
   GtkWidget *container;
   
   container = glade_xml_get_widget( window->glade, "info_box_box" );
   g_assert( container );
   gtk_container_foreach( GTK_CONTAINER( container ), (GtkCallback)gtk_widget_destroy, NULL );
}   

void ucview_window_set_info_box( UCViewWindow *window, GtkWidget *info_box )
{
   GtkWidget *container;
   
   container = glade_xml_get_widget( window->glade, "info_box_box" );
   g_assert( container );
   gtk_container_foreach( GTK_CONTAINER( container ), (GtkCallback)gtk_widget_destroy, NULL );
   
   gtk_container_add( GTK_CONTAINER( container ), info_box );
}


static void hide_image_info_box_toggled_cb( GtkToggleButton *button, UCViewWindow *window )
{
   gconf_client_set_bool( window->client, 
			  UCVIEW_GCONF_DIR "/hide_image_saved_box", 
			  gtk_toggle_button_get_active( button ), 
			  NULL );
}

static gboolean default_save_handler( unicap_data_buffer_t *buffer, gchar *path, gchar *filetype, UCViewWindow *window, GError **err )
{
      GdkPixbuf *pixbuf;
      gboolean ret = FALSE;
   
      unicap_data_buffer_t still_image_buffer;
      
      unicap_copy_format( &still_image_buffer.format, &buffer->format );
      still_image_buffer.format.fourcc = UCIL_FOURCC( 'R', 'G', 'B', '3' );
      still_image_buffer.format.bpp = 24;
      still_image_buffer.format.buffer_size = still_image_buffer.format.size.width * 
	 still_image_buffer.format.size.height * 3;
      still_image_buffer.buffer_size = still_image_buffer.format.buffer_size;
      still_image_buffer.data = malloc( still_image_buffer.format.buffer_size );
      
      ucil_convert_buffer( &still_image_buffer, buffer );

      pixbuf = gdk_pixbuf_new_from_data( still_image_buffer.data, 
					 GDK_COLORSPACE_RGB, 
					 0, 
					 8, 
					 still_image_buffer.format.size.width, 
					 still_image_buffer.format.size.height, 
					 still_image_buffer.format.size.width * 3, 
					 NULL, 
					 NULL );
      if( pixbuf ){
	 ret = gdk_pixbuf_save( pixbuf, path, filetype, err, NULL );
	 g_object_unref( pixbuf );
      }
      
      return ret;
}


static void predisplay_cb( UnicapgtkVideoDisplay *ugtk, unicap_data_buffer_t *buffer, UCViewWindow *window )
{
   if( window->still_image || window->clipboard_copy_image )
   {

      ucil_convert_buffer( &window->still_image_buffer, buffer );				 
					 
      if( window->still_image ){
	 ImageFileTypeDetail *detail;
	 detail = get_image_file_type_detail( window, window->still_image_file_type );
	 if( detail ){
/* 	    g_signal_emit( G_OBJECT( window ), ucview_signals[ UCVIEW_SAVE_IMAGE_SIGNAL ], 0, window->still_image_buffer, window->still_image_path ); */
/* 	    detail->handler( buffer, window->still_image_path, window->still_image_file_type, detail->user_ptr, NULL ); */
/* 	    g_signal_emit( G_OBJECT( window ), ucview_signals[ UCVIEW_IMAGE_FILE_CREATED_SIGNAL ], 0, window->still_image_path ); */

	    if( !window->time_lapse && !gconf_client_get_bool( window->client, UCVIEW_GCONF_DIR "/hide_image_saved_box", NULL ) )
	    {
	       GtkWidget *vbox;
	       GtkWidget *info_text;
	       GtkWidget *info_box;
	       GtkWidget *check_button;
	       gchar *message;
	       
	       vbox = gtk_vbox_new( FALSE, 6 );
	       
	       g_assert( 
		  asprintf( &message, 
			    _("Picture saved to: \n<i>%s</i>"), 
			    window->still_image_path ) >= 0 );
	       
	       info_text = gtk_label_new( message );
	       g_object_set( info_text, "use-markup", TRUE, NULL );
	       g_free( message );

	       gtk_box_pack_start( GTK_BOX( vbox ), info_text, FALSE, TRUE, 0 );

	       check_button = gtk_check_button_new_with_label( _("Do not show this message again") );
	       g_signal_connect( check_button, "toggled", (GCallback)hide_image_info_box_toggled_cb, window );
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
/* 	    sidebar_add_image( SIDEBAR( window->sidebar ), buffer, window->still_image_path ); */
	 }
	 
	 
	 if( window->clipboard_copy_image )
	 {
	    GtkClipboard *clipboard;
	    GdkPixbuf *pixbuf;

	    g_message( "Copy image" );
	    
	    pixbuf = gdk_pixbuf_new_from_data( window->still_image_buffer.data, 
					       GDK_COLORSPACE_RGB, 
					       0, 
					       8, 
					       window->still_image_buffer.format.size.width, 
					       window->still_image_buffer.format.size.height, 
					       window->still_image_buffer.format.size.width * 3, 
					       NULL, 
					       NULL );
	    clipboard = gtk_clipboard_get( GDK_SELECTION_CLIPBOARD );
	    
	    gtk_clipboard_set_image( clipboard, pixbuf );
	    g_object_unref( pixbuf );
	 }
      }
      
      window->still_image = FALSE;
      window->clipboard_copy_image = FALSE;
   }

   g_signal_emit( G_OBJECT( window ), ucview_signals[ UCVIEW_DISPLAY_IMAGE_SIGNAL ], 0, (gpointer)buffer );


}

static void device_removed_response_cb( GtkWidget *box, gint response, UCViewWindow *window )
{
   
   switch( response ){
   case GTK_RESPONSE_CLOSE:
      gtk_main_quit();
      break;
      
   case UCVIEW_INFO_BOX_RESPONSE_ACTION1:
      change_device_cb( NULL, window );
      break;
      
   default:
      break;
   }

   gtk_widget_destroy( box );
}


/*
  device_removed_cb: 
  
  Called on device removal in a unicap thread context
 */
static void device_removed_cb( unicap_event_t event, unicap_handle_t handle, UCViewWindow *window )
{
   gdk_threads_enter();

   GtkWidget *info_box;
   GtkWidget *info_text;
   gchar *message;
   unicap_device_t device;
   
   unicap_get_device( handle, &device );
   
   message = g_strdup_printf( _("<b>Device '%s' disconnected</b>"), device.identifier );
   info_text = gtk_label_new( message );
   gtk_misc_set_alignment( GTK_MISC( info_text ), 0.0f, 0.0f );
   g_free( message );   
   g_object_set( info_text, "use-markup", TRUE, NULL );
   
   info_box = g_object_new( UCVIEW_INFO_BOX_TYPE, "image", gtk_image_new_from_stock( GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_DIALOG ),
			    "text", info_text, NULL );
   ucview_info_box_add_action_widget( UCVIEW_INFO_BOX( info_box ), 
				      gtk_button_new_with_label( _("Select Device") ), 
				      UCVIEW_INFO_BOX_RESPONSE_ACTION1 );
   ucview_info_box_add_action_widget( UCVIEW_INFO_BOX( info_box ),
				      gtk_button_new_from_stock( GTK_STOCK_QUIT ),
				      GTK_RESPONSE_CLOSE );
   g_signal_connect( info_box, "response", G_CALLBACK( device_removed_response_cb ), window );
   gtk_widget_show_all( info_box );
   ucview_window_set_info_box( window, info_box );

   gdk_threads_leave();
}



static void new_frame_cb( unicap_event_t event, unicap_handle_t handle, unicap_data_buffer_t *buffer, UCViewWindow *window )
{
   gdouble ival;
   static int i = 0;
   int count = 0;
   int j;
   gdouble tmp = 0;
   GList *e;
   
   g_mutex_lock( window->mutex );

   ival = g_timer_elapsed( window->priv->fps_timer, NULL );
   g_timer_start( window->priv->fps_timer );
   
   window->priv->fps_times[i++] = ival;
   i = i % FPS_VALUES;
   for( j = 0; j < FPS_VALUES; j++ )
   {
      if( window->priv->fps_times[j] != 0 ){
	 count++;
	 tmp += window->priv->fps_times[j];
      }
   }
   tmp /= count;
   
   window->priv->fps = 1.0 / tmp;

   for( e = g_list_first( window->modules ); e; e = g_list_next( e ) )
   {
      UCViewPluginData *plugin;
      
      plugin = e->data;
      crash_data.plugin = plugin;
      if( plugin->enable && plugin->plugin_data->new_frame_cb )
      {
	 plugin->plugin_data->new_frame_cb( event, handle, buffer, plugin->plugin_data->user_ptr );
      }
      crash_data.plugin = NULL;
   }

   if( window->time_lapse )
   {
      gboolean stop = FALSE;     
      
      if( window->time_lapse_first_frame || ( ( g_timer_elapsed( window->time_lapse_timer, NULL ) * 1000.0 ) >= window->time_lapse_delay ) )
      {
	 window->time_lapse_frames++;
	 if( window->time_lapse_mode == UCVIEW_TIME_LAPSE_VIDEO )
	 {
	    if( window->record )
	    {
	       ucil_encode_frame( window->video_object, buffer );
	    }
	 }
	 else
	 {
	    GtkAction *save_action = gtk_ui_manager_get_action( window->priv->ui, "/MenuBar/FileMenu/SaveImage" );
	    gdk_threads_enter();
	    gtk_action_activate( save_action );
	    gdk_threads_leave();
	 }
	 g_timer_start( window->time_lapse_timer );
	 window->time_lapse_first_frame = FALSE;
      }

      if( window->time_lapse_total_time && 
	  ( ( g_timer_elapsed( window->time_lapse_total_timer, NULL ) * 1000.0 ) >= window->time_lapse_total_time ) )
      {
	 stop = TRUE;
      }
      
      if( window->time_lapse_total_frames && ( window->time_lapse_frames >= window->time_lapse_total_frames ) )
      {
	 stop = TRUE;
      }
      
      if( stop )
      {
	 GtkAction *time_lapse_action = gtk_ui_manager_get_action( window->priv->ui, "/MenuBar/EditMenu/TimeLapse" );
	 gdk_threads_enter();
	 gtk_toggle_action_set_active( GTK_TOGGLE_ACTION( time_lapse_action ), FALSE );
	 gdk_threads_leave();
      }
   }
   else if( window->record )
   {
      ucil_encode_frame( window->video_object, buffer );
   }

   for( e = g_list_first( window->modules ); e; e = g_list_next( e ) )
   {
      UCViewPluginData *plugin;
      
      plugin = e->data;
      crash_data.plugin = plugin;
      if( plugin->enable && plugin->plugin_data->display_frame_cb )
      {
	 plugin->plugin_data->display_frame_cb( event, handle, buffer, plugin->plugin_data->user_ptr );
      }
      crash_data.plugin = NULL;
   }

   g_mutex_unlock( window->mutex );

}


static void build_main_window( UCViewWindow *window )
{
   GtkWidget *hbox;
   GtkWidget *box;
   GtkWidget *toolbar;
   GdkPixbuf *pixbuf;
   GtkWidget *settings_dialog;
   GtkWidget *frame;
   GtkWidget *eventbox;

   pixbuf = gdk_pixbuf_new_from_inline( -1, icon_ucview_data, FALSE, NULL );
   gtk_window_set_icon( GTK_WINDOW( window ), pixbuf );
   g_object_unref( G_OBJECT( pixbuf ) );

   hbox = glade_xml_get_widget( window->glade, "ucview_main_hbox" );
   gtk_container_add( GTK_CONTAINER( window ), hbox );
   
   window->priv->ui = gui_create_manager( window );
   box = glade_xml_get_widget( window->glade, "ucview_main_menubar_box" );
   gtk_container_foreach( GTK_CONTAINER( box ), (GtkCallback)gtk_widget_destroy, NULL );
   gtk_box_pack_start( GTK_BOX( box ), gtk_ui_manager_get_widget( window->priv->ui, "/MenuBar" ), TRUE, TRUE, 0 );
   gtk_window_add_accel_group( GTK_WINDOW( window ), gtk_ui_manager_get_accel_group( window->priv->ui ) );

   box = glade_xml_get_widget( window->glade, "ucview_main_toolbar_box" );
   gtk_container_foreach( GTK_CONTAINER( box ), (GtkCallback)gtk_widget_destroy, NULL );
   toolbar = gtk_ui_manager_get_widget( window->priv->ui, "/ToolBar" );
   gtk_box_pack_start( GTK_BOX( box ), toolbar, TRUE, TRUE, 0 );


   window->vtoolbarwindow = gtk_window_new( GTK_WINDOW_POPUP );
   eventbox = gtk_event_box_new();
   frame = gtk_frame_new( NULL );
   gtk_window_set_decorated( GTK_WINDOW( window->vtoolbarwindow ), FALSE );
   gtk_window_set_skip_taskbar_hint( GTK_WINDOW( window->vtoolbarwindow ), TRUE );
   toolbar = gtk_ui_manager_get_widget( window->priv->ui, "/VToolBar" );
   gtk_toolbar_set_orientation( GTK_TOOLBAR( toolbar ), GTK_ORIENTATION_VERTICAL );
   gtk_toolbar_set_show_arrow( GTK_TOOLBAR( toolbar ), FALSE );
   gtk_container_add( GTK_CONTAINER( frame ), toolbar );
   gtk_container_add( GTK_CONTAINER( eventbox ), frame );
   gtk_container_add( GTK_CONTAINER( window->vtoolbarwindow ), eventbox );
   gtk_window_set_keep_above( GTK_WINDOW( window->vtoolbarwindow ), TRUE );
   gtk_window_set_default_size( GTK_WINDOW( window->vtoolbarwindow ), 10, 10 );

   g_signal_connect( eventbox, "enter-notify-event", (GCallback)fs_toolbar_enter_notify_cb, window );


   settings_dialog = settings_dialog_new( window );
   g_signal_connect_swapped( G_OBJECT( window ), "destroy", G_CALLBACK( gtk_widget_destroy ), settings_dialog );
   g_signal_connect( G_OBJECT( settings_dialog ), "delete-event", G_CALLBACK( gtk_widget_hide_on_delete ), NULL );   
   gtk_window_set_transient_for( GTK_WINDOW( settings_dialog ), GTK_WINDOW( window ) );
   window->settings_dialog = settings_dialog;
   
   window->display_window = glade_xml_get_widget( window->glade, "ucview_display_scrolledwindow" );
   window->priv->statusbar = glade_xml_get_widget( window->glade, "ucview_statusbar" );

   ucview_window_register_image_file_type( window, "jpeg", "jpg", (UCViewSaveImageFileHandler)default_save_handler, window );
   ucview_window_register_image_file_type( window, "png", "png", (UCViewSaveImageFileHandler)default_save_handler, window );
   ucview_window_register_image_file_type( window, "bmp", "bmp", (UCViewSaveImageFileHandler)default_save_handler, window );


}

static gboolean resize_window( UCViewWindow *ucv )
{
   // This is a bit of a hack: 
   // Reset the size request of the display window to a small value to
   // alow the user to make the window smaller. 
   // The display request is set to the full resolution of the video
   // format previously so that the window is created with the correct size.

   gtk_widget_set_size_request( ucv->display_window, 64, 64 );
   return FALSE;
}

GtkWidget *ucview_window_new( unicap_handle_t handle )
{
   UCViewWindow *window;
   
   window = g_object_new( UCVIEW_WINDOW_TYPE, NULL );
   if( handle )
   {
      g_object_set( window, "unicap-handle", handle, NULL );
   }
   
   
   return GTK_WIDGET( window );
}



void ucview_window_set_fullscreen( UCViewWindow *window, gboolean fullscreen )
{
   GtkWidget *eventbox;
   GtkWidget *htoolbar;
   GtkWidget *vtoolbar;
   GtkWidget *menubar;
   
   htoolbar = gtk_ui_manager_get_widget( window->priv->ui, "/ToolBar" );
   vtoolbar = gtk_ui_manager_get_widget( window->priv->ui, "/VToolBar" );
   menubar = gtk_ui_manager_get_widget( window->priv->ui, "/MenuBar" );
   eventbox = glade_xml_get_widget( window->glade, "ucview_main_eventbox" );
   g_assert( htoolbar && vtoolbar && menubar && eventbox );
   gtk_widget_show( eventbox );
   
   window->is_fullscreen = fullscreen;
	 
   if( !fullscreen )
   {
      gtk_window_unfullscreen( GTK_WINDOW( window ) );
      gconf_client_set_bool( window->client, UCVIEW_GCONF_DIR "/fullscreen", FALSE, NULL );
      gtk_widget_hide( window->vtoolbarwindow );
      if( gconf_client_get_bool( window->client, UCVIEW_GCONF_DIR "/show_toolbar", NULL ) )
      {
	 gtk_widget_show( htoolbar );
      }

      if( gconf_client_get_bool( window->client, UCVIEW_GCONF_DIR "/show_sidebar", NULL ) )
      {
	 gtk_widget_show( glade_xml_get_widget( window->glade, "ucview_sidebar_box" ) );
      }
      
      gtk_widget_show( menubar );
      gtk_widget_show( window->priv->statusbar );
      gtk_widget_modify_bg( window->display_ebox, GTK_STATE_NORMAL, NULL );
   }
   else
   {
      GdkColormap *colormap;
      GdkColor color;      
      GtkRequisition toolbar_req;
      GdkScreen *screen;

      color.red = color.green = color.blue = 0;
      colormap = gtk_widget_get_colormap( window->display_ebox );
      gdk_rgb_find_color( colormap, &color );
      gtk_widget_modify_bg( window->display_ebox, GTK_STATE_NORMAL, &color );
      gtk_window_present( GTK_WINDOW( window->vtoolbarwindow ) );

      gtk_window_fullscreen( GTK_WINDOW( window ) );
      screen = gtk_widget_get_screen( GTK_WIDGET( window ) );
      gtk_widget_show_all( window->vtoolbarwindow );
      gtk_widget_size_request( window->vtoolbarwindow, &toolbar_req );
      gtk_window_move( GTK_WINDOW( window->vtoolbarwindow ), 0, gdk_screen_get_height( screen ) / 2 - toolbar_req.height / 2 );

      gconf_client_set_bool( window->client, UCVIEW_GCONF_DIR "/fullscreen", TRUE, NULL );
      gtk_widget_hide( menubar );
      gtk_widget_hide( window->priv->statusbar );
      gtk_widget_hide( htoolbar );
      gtk_widget_hide( glade_xml_get_widget( window->glade, "ucview_sidebar_box" ) );
      gtk_widget_show_all( vtoolbar );
   }   
}

GtkUIManager *ucview_window_get_ui_manager( UCViewWindow *ucv )
{
   return ucv->priv->ui;
}

GtkWidget *ucview_window_get_statusbar( UCViewWindow *ucv )
{
   return ucv->priv->statusbar;
}

void ucview_window_set_device_dialog (UCViewWindow *ucv, GtkWidget *dlg)
{
	ucv->priv->device_dialog = dlg;
}

GtkWidget *ucview_window_get_device_dialog (UCViewWindow *ucv)
{
	return ucv->priv->device_dialog;
}

static gboolean fps_timeout_cb( UCViewWindow *window )
{
   gchar fps_message[64];
   gdouble ival;
   gdouble fps;
   
   gtk_statusbar_pop( GTK_STATUSBAR( ucview_window_get_statusbar( UCVIEW_WINDOW( window ) ) ), 0 );

   ival = g_timer_elapsed( window->priv->fps_timer, NULL );
   fps = 1.0 / ((ival>0.0) ? ival : 1.0 );
   if( ( ival > window->priv->fps + 1.0 ) && ( window->priv->fps > 0.0 ) ){
      gtk_statusbar_push( GTK_STATUSBAR( ucview_window_get_statusbar( UCVIEW_WINDOW( window ) ) ), 0, 
			  _("Video stream has stalled") );
   } else {
      sprintf( fps_message, _("%.1f FPS"), window->priv->fps );
      gtk_statusbar_push( GTK_STATUSBAR( ucview_window_get_statusbar( UCVIEW_WINDOW( window ) ) ), 0, fps_message );
   }

   return TRUE;
}

void ucview_window_enable_fps_display( UCViewWindow *window, gboolean enable )
{
   if( enable ){
      window->priv->fps_timeout = g_timeout_add( 250, (GSourceFunc) fps_timeout_cb, window );
   } else {
      if( window->priv->fps_timeout != 0 ){
	 g_source_remove( window->priv->fps_timeout );
	 window->priv->fps_timeout = 0;
	 gtk_statusbar_pop( GTK_STATUSBAR( ucview_window_get_statusbar( UCVIEW_WINDOW( window ) ) ), 0 );
      }
   }
}

