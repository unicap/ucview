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
#include <locale.h>


#include "ucview-window.h"
#include "icons.h"
#include "ucview-device-dialog.h"
#include "callbacks.h"


static gboolean create_main_window()
{
   GtkWidget *ucv;
   GtkWidget *device_dialog;
   unicap_handle_t handle;
   unicap_format_t format;

   device_dialog = g_object_new( UCVIEW_DEVICE_DIALOG_TYPE, "restore-device", TRUE, NULL );
   gtk_widget_show_all( device_dialog );
   handle = ucview_device_dialog_run( UCVIEW_DEVICE_DIALOG( device_dialog ) );
   ucview_device_dialog_get_format( UCVIEW_DEVICE_DIALOG( device_dialog ), &format );
   gtk_widget_hide( device_dialog );

   if( !handle )
   {
      return FALSE;
   }

   icons_add_stock_items();
   ucv = ucview_window_new( handle );
   g_object_set_data (device_dialog, "ucview_window", ucv);
   g_signal_connect( UCVIEW_DEVICE_DIALOG(device_dialog)->ok_button, "clicked", (GCallback)device_dialog_ok_clicked_cb, device_dialog );
   ucview_window_set_device_dialog (ucv, device_dialog);
   ucview_window_set_video_format( UCVIEW_WINDOW( ucv ), &format );
   ucview_window_start_live( UCVIEW_WINDOW( ucv ) );
   
   gtk_widget_show( ucv );
/*    g_object_set( ucv, "unicap-handle", handle, NULL ); */
   g_signal_connect( G_OBJECT( ucv ), "destroy", G_CALLBACK( gtk_main_quit ), NULL );
   
   return TRUE;
}

int main( int argc, char**argv )
{

   g_thread_init(NULL);
   gdk_threads_init();
   gdk_threads_enter();
   gtk_init (&argc, &argv);

   setlocale( LC_ALL, "" );

   bindtextdomain( GETTEXT_PACKAGE, UCVIEW_LOCALEDIR );
   bind_textdomain_codeset( GETTEXT_PACKAGE, "UTF-8" );
   textdomain( GETTEXT_PACKAGE );

   if( create_main_window() ){
      gtk_main();
   }

   gdk_threads_leave();
   return 0;
}

