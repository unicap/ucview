/*
** ucview_device_dialog.c
** 
** Made by (Arne Caspari)
** Login   <arne@arne-laptop>
** 
** Started on  Wed Nov 15 07:19:16 2006 Arne Caspari
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


#include "ucview-device-dialog.h"

#include <gconf/gconf-client.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <unicapgtk.h>

#include <string.h>

#include "ucview-window.h"
#include "user_config.h"
#include "worker.h"

static void ucview_device_dialog_class_init( UCViewDeviceDialogClass *klass );
static void ucview_device_dialog_init( UCViewDeviceDialog *dlg );
static void ucview_device_dialog_set_property( GObject *obj, guint property_id, const GValue *value, GParamSpec *pspec );
static void ucview_device_dialog_get_property( GObject *obj, guint property_id, GValue *value, GParamSpec *pspec );
static void ucview_device_dialog_destroy( GtkObject     *object );

static void device_change_cb( UnicapgtkDeviceSelection *selecion, gchar *device_id, UCViewDeviceDialog *dlg );
static void format_change_cb( UnicapgtkVideoFormatSelection *sel, unicap_format_t *format, UCViewDeviceDialog *dlg );
static void delete_cb( GtkWidget *window, GdkEvent *event, UCViewDeviceDialog *dlg );
static void ok_cb( GtkWidget *button, UCViewDeviceDialog *dlg );


G_DEFINE_TYPE( UCViewDeviceDialog, ucview_device_dialog, GTK_TYPE_WINDOW );

enum 
{
   PROP_0 = 0, 
   PROP_RESTORE_DEVICE,
};

static void ucview_device_dialog_class_init( UCViewDeviceDialogClass *klass )
{
   GObjectClass *object_class = G_OBJECT_CLASS( klass );
   GtkObjectClass *gtk_object_class = GTK_OBJECT_CLASS( klass );
   
   object_class->set_property = ucview_device_dialog_set_property;
   object_class->get_property = ucview_device_dialog_get_property;
   gtk_object_class->destroy  = ucview_device_dialog_destroy;

   g_object_class_install_property( object_class, PROP_RESTORE_DEVICE, 
				    g_param_spec_boolean( "restore-device", NULL, NULL, TRUE, 
							  G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY ) );
}

static void create_dlg( UCViewDeviceDialog *dlg, gboolean restore_device )
{
   unicap_handle_t handle = NULL;   
   GtkWidget *vbox;
   GtkWidget *label;
   GtkWidget *device_selection;
   GtkWidget *format_selection;
   GtkWidget *table;
   GtkWidget *bbox;

   if( restore_device )
   {
      GConfClient *client;
      
      client = gconf_client_get_default();
      if( gconf_client_get_bool( client, UCVIEW_GCONF_DIR "/remember_device", NULL ) )
      {
	 unicap_device_t device;
	 
	 handle = user_config_try_restore();
	 
	 if( handle )
	 {
	    unicap_get_device( handle, &device );
	    
	    if( unicap_is_stream_locked( &device ) )
	    {
	       unicap_close( handle );
	       handle = NULL;
	    }
	 }
      }
      g_object_unref( client );
   }
   
   dlg->handle = handle;

   gtk_window_set_title( GTK_WINDOW( dlg ), _("Select Device") );
   vbox = gtk_vbox_new( FALSE, FALSE );
   gtk_container_add( GTK_CONTAINER( dlg ), vbox );
   table = gtk_table_new( 2, 2, FALSE );
   gtk_box_pack_start( GTK_BOX( vbox ), table, FALSE, FALSE, 12 );
   label = gtk_label_new( NULL );
   gtk_label_set_markup( GTK_LABEL( label ), _("<b>Device:</b>") );
   gtk_table_attach_defaults( GTK_TABLE( table ), label, 0, 1, 0, 1 );
   device_selection = unicapgtk_device_selection_new( FALSE );
   dlg->device_selection = device_selection;
   unicapgtk_device_selection_rescan( UNICAPGTK_DEVICE_SELECTION( device_selection ) );
   gtk_table_attach_defaults( GTK_TABLE( table ), device_selection, 1, 2, 0, 1 );

   label = gtk_label_new( NULL );
   gtk_label_set_markup( GTK_LABEL( label ), _("<b>Video Format:</b>") );
   gtk_table_attach_defaults( GTK_TABLE( table ), label, 0, 1, 1, 2 );
   format_selection = unicapgtk_video_format_selection_new( );
   dlg->format_selection = format_selection;
   gtk_table_attach_defaults( GTK_TABLE( table ), format_selection, 1, 2, 1, 2 );

   gtk_table_set_row_spacings( GTK_TABLE( table ), 5 );
   gtk_table_set_col_spacings( GTK_TABLE( table ), 5 );

   dlg->check_button = gtk_check_button_new_with_label( _("Remember selection and use as default in the future") );
   gtk_box_pack_start( GTK_BOX( vbox ), dlg->check_button, FALSE, FALSE, 12 );

   bbox = gtk_hbutton_box_new( );
   gtk_box_pack_start( GTK_BOX( vbox ), bbox, FALSE, FALSE, 0 );
   
   dlg->rescan_button = gtk_button_new_from_stock( GTK_STOCK_REFRESH );
   gtk_container_add( GTK_CONTAINER( bbox ), dlg->rescan_button );

   dlg->ok_button = gtk_button_new_from_stock( GTK_STOCK_OK );
   gtk_container_add( GTK_CONTAINER( bbox ), dlg->ok_button );
   gtk_widget_grab_focus( dlg->ok_button );

   g_signal_connect( G_OBJECT( device_selection ), "unicapgtk_device_selection_changed", (GCallback)device_change_cb, dlg );
   g_signal_connect( G_OBJECT( format_selection ), "unicapgtk_video_format_changed", (GCallback)format_change_cb, dlg );
   g_signal_connect( G_OBJECT( dlg->ok_button ), "clicked", (GCallback)ok_cb, dlg );
   g_signal_connect_swapped( dlg->rescan_button, "clicked", (GCallback)unicapgtk_device_selection_rescan, device_selection );


   if( !handle )
   {
      unicapgtk_device_selection_set_device( UNICAPGTK_DEVICE_SELECTION( device_selection ), NULL );
   }else{
      unicap_format_t format;
      
      if( SUCCESS( unicap_get_format( handle, &format ) ) ){
	 unicap_copy_format( &dlg->format, &format );
      }
      
   }

   gtk_widget_show_all( vbox );
}


static void ucview_device_dialog_init( UCViewDeviceDialog *dlg )
{
   dlg->handle = NULL;

}

static void ucview_device_dialog_destroy( GtkObject *object )
{
   UCViewDeviceDialog *dlg = UCVIEW_DEVICE_DIALOG( object );

   gtk_container_foreach( GTK_CONTAINER( object ), (GtkCallback) gtk_widget_destroy, NULL );
   
   if( dlg->handle )
   {
      unicap_close( dlg->handle );
      dlg->handle = NULL;
   }
   
}


static void ucview_device_dialog_set_property( GObject *obj, guint property_id, const GValue *value, GParamSpec *pspec )
{
   UCViewDeviceDialog *dlg = UCVIEW_DEVICE_DIALOG( obj );


   switch( property_id )
   {
      case PROP_RESTORE_DEVICE:
      {
	 dlg->restore_device = g_value_get_boolean( value );
	 create_dlg( dlg, dlg->restore_device );
      }
      break;
      
      default:
	 G_OBJECT_WARN_INVALID_PROPERTY_ID( obj, property_id, pspec );
	 break;
   }
}

static void ucview_device_dialog_get_property( GObject *obj, guint property_id, GValue *value, GParamSpec *pspec )
{   
   UCViewDeviceDialog *dlg = UCVIEW_DEVICE_DIALOG( obj );
   
   switch( property_id )
   {
      case PROP_RESTORE_DEVICE:
      {
	 g_value_set_boolean( value, dlg->restore_device );
      }
      break;

      default:
	 G_OBJECT_WARN_INVALID_PROPERTY_ID( obj, property_id, pspec );
	 break;
   }   
}

static void device_change_cb( UnicapgtkDeviceSelection *selecion, gchar *device_id, UCViewDeviceDialog *dlg )
{
   unicap_handle_t handle;
   unicap_device_t device;
   
   if( dlg->handle )
   {
      unicap_close( dlg->handle );
      dlg->handle = NULL;
   }

   unicap_void_device( &device );
   strcpy( device.identifier, device_id );
   unicap_enumerate_devices( &device, &device, 0 );
   
   if( SUCCESS( unicap_open( &handle, &device ) ) )
   {
      // dummy action for STK camera
/*       run_worker( (void*(*)(void*))device_change_worker, handle ); */

      unicapgtk_video_format_selection_set_handle( UNICAPGTK_VIDEO_FORMAT_SELECTION( dlg->format_selection ), 
						   handle );

      unicap_close( handle );
   }
}

static void format_change_cb( UnicapgtkVideoFormatSelection *sel, unicap_format_t *format, UCViewDeviceDialog *dlg )
{
   unicap_copy_format( &dlg->format, format );
}

static void delete_cb( GtkWidget *window, GdkEvent *event, UCViewDeviceDialog *dlg )
{
   dlg->delete = TRUE;
}

static void ok_cb( GtkWidget *button, UCViewDeviceDialog *dlg )
{
   unicap_handle_t handle;
   handle = unicapgtk_video_format_selection_get_handle( UNICAPGTK_VIDEO_FORMAT_SELECTION( dlg->format_selection ) );
   dlg->handle = handle;
}

void ucview_device_dialog_get_format( UCViewDeviceDialog *dlg, unicap_format_t *format )
{
   unicap_copy_format( format, &dlg->format );
}

unicap_handle_t ucview_device_dialog_get_handle( UCViewDeviceDialog *dlg )
{
   return unicap_clone_handle( dlg->handle );
}


unicap_handle_t ucview_device_dialog_run( UCViewDeviceDialog *dlg )
{
   GConfClient *client;
   
   
   client = gconf_client_get_default();

   g_signal_connect( G_OBJECT( dlg ), "delete-event", (GCallback)delete_cb, dlg );

   if( !dlg->handle )
   {
      gtk_widget_show_all( GTK_WIDGET( dlg ) );
      gtk_window_present( GTK_WINDOW( dlg ) );
      while( ( !dlg->handle ) && ( !dlg->delete ) )
      {
	 gtk_main_iteration();
      }

      gconf_client_set_bool( client, 
			     UCVIEW_GCONF_DIR "/remember_device", 
			     gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( dlg->check_button ) ), 
			     NULL ); 

   }
   
   if( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( dlg->check_button ) ) && dlg->handle )
   {
      user_config_store_default( dlg->handle );
   }

   g_object_unref( client );

   return unicap_clone_handle( dlg->handle );
}


void ucview_device_dialog_select_device( UCViewDeviceDialog *dlg, unicap_device_t *device )
{
   unicapgtk_device_selection_set_device( UNICAPGTK_DEVICE_SELECTION( dlg->device_selection ), device );
}
