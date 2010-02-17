/*
** checkperm.c
** 
** Made by (Arne Caspari)
** Login   <arne@arne-laptop>
** 
** Started on  Tue Jan 30 07:28:04 2007 Arne Caspari
** Last update Wed Feb  7 10:41:32 2007 Arne Caspari
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

#include <gtk/gtk.h>
#include <gconf/gconf-client.h>


#include <unistd.h>

#include "checkperm.h"
#include "ucview.h"
#include "ucview-window.h"

extern const unsigned int unicap_major_version;
extern const unsigned int unicap_minor_version;
extern const unsigned int unicap_micro_version;
extern const unsigned int unicapgtk_major_version;
extern const unsigned int unicapgtk_minor_version;
extern const unsigned int unicapgtk_micro_version;



#define RAW1394PATH "/dev/raw1394"
#define RAW1394_NOT_ACCESSIBLE_MSG "You do not have the right permissions to access /dev/raw1394!\n" \
                                   "IEEE1394 ( FireWire ) cameras will not work until the permissions are set correctly.\n"
#define RAW1394_NOT_EXISTS_MSG     "The device file '/dev/raw1394' does not exist.\n"\
                                   "This usually means that the 'raw1394' module is not loaded.\n" \
                                   "If you are going to use an IEEE1394 ( FireWire ) camera, you should try the following: \n" \
                                   " - check that the camera is connected and has power\n" \
                                   " - open a shell and enter ( as root ): 'modprobe raw1394'\n"


static void keep_dialog_toggled( GtkToggleButton *toggle, GConfClient *client )
{
   gconf_client_set_bool( client, UCVIEW_GCONF_DIR "/hide_startup_check_dialog", 
			  !gtk_toggle_button_get_active( toggle ), NULL );
}


static GtkWidget *create_message_dialog( const gchar *message, GConfClient *client )
{
   GtkWidget *dialog;
   GtkWidget *label;
   GtkWidget *toggle;
   GtkWidget *image;
   GtkWidget *hbox;

   gchar text[512];

   sprintf( text, "%s\nunicap version: %d.%d.%d\nunicapgtk version: %d.%d.%d", 
	    message, 
	    unicap_major_version, unicap_minor_version, unicap_micro_version, 
	    unicapgtk_major_version, unicapgtk_minor_version, unicapgtk_micro_version );
   
   dialog = gtk_dialog_new_with_buttons( "Message", 
					 NULL, 
					 GTK_DIALOG_MODAL, 
					 GTK_STOCK_OK, 
					 GTK_RESPONSE_NONE, 
					 NULL );
   
   hbox = gtk_hbox_new( 0, 0 );
   gtk_container_add( GTK_CONTAINER( GTK_DIALOG( dialog )->vbox ), hbox );

   image = gtk_image_new_from_stock( GTK_STOCK_DIALOG_INFO, GTK_ICON_SIZE_DIALOG );
   gtk_box_pack_start( GTK_BOX( hbox ), image, TRUE, TRUE, 0 );

   label = gtk_label_new( text );
   gtk_box_pack_start( GTK_BOX( hbox ), label, TRUE, TRUE, 0 );
   
   toggle = gtk_check_button_new_with_label( "Show this dialog again" );
   gtk_container_add( GTK_CONTAINER( GTK_DIALOG( dialog )->vbox ), toggle );
   
   g_signal_connect( G_OBJECT( toggle ), "toggled", (GCallback)keep_dialog_toggled, client );

   gtk_widget_show_all( dialog );

   return dialog;
}



gboolean check_unicap_env( GConfClient *client )
{
   GtkWidget *dlg = NULL;
   
   if( !gconf_client_get_bool( client, UCVIEW_GCONF_DIR "/hide_startup_check_dialog", NULL ) )
   {

      if( !access( RAW1394PATH, F_OK ) )
      {
	 if( access( RAW1394PATH, R_OK | W_OK ) )
	 {
	    dlg = create_message_dialog( RAW1394_NOT_ACCESSIBLE_MSG, client );
	 }
      }
      else
      {
	 dlg = create_message_dialog( RAW1394_NOT_EXISTS_MSG, client );
      }
   
      if( dlg )
      {
	 gtk_dialog_run( GTK_DIALOG( dlg ) );
      }
   
      gtk_widget_destroy( dlg );
   }

   return( dlg == NULL );
}
