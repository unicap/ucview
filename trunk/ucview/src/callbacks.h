/*
** callbacks.h
** 
** Made by Arne Caspari
** Login   <arne@arne-laptop>
** 
** Started on  Wed Feb 21 18:18:50 2007 Arne Caspari
** Last update Wed Feb 21 18:35:52 2007 Arne Caspari
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

#ifndef   	CALLBACKS_H_
# define   	CALLBACKS_H_

#include <glib.h>
#include <gtk/gtk.h>
#include <unicap.h>
#include <unicapgtk.h>
#include <ucil.h>

#include "ucview-window.h"
#include "ucview-device-dialog.h"

void gconf_notification_cb( GConfClient *client, guint cnxn_id, GConfEntry *entry, UCViewWindow *window );
gboolean fs_toolbar_enter_notify_cb( GtkWidget *widget, GdkEventCrossing *event, UCViewWindow *window );
void fps_toggled_cb( GtkAction *action, UCViewWindow *window );
void time_lapse_toggled_cb( GtkAction *action, UCViewWindow *window );
void record_toggled_cb( GtkAction *action, UCViewWindow *window );
void play_toggled_cb( GtkAction *action, UCViewWindow *window );
void clipboard_copy_image_cb( GtkAction *action, UCViewWindow *window );
void save_still_image_cb( GtkAction *action, UCViewWindow *window );
void color_mag_toggled_cb( GtkAction *action, UCViewWindow *window );
void pause_state_toggled_cb( GtkAction *action, UCViewWindow *window );
void showhide_property_dialog( GtkToggleToolButton *toggle, GtkWidget *property_dialog );
gboolean property_dialog_hide( UnicapgtkPropertyDialog *property_dialog, GdkEvent *event, GtkToggleToolButton *toggle );
void display_property_dialog_cb( GtkAction *action, UCViewWindow *window );
void display_settings_dialog_cb( GtkAction *action, UCViewWindow *window );

void change_device_cb( GtkAction *action, UCViewWindow *window );
void device_dialog_ok_clicked_cb( GtkButton *button, UCViewDeviceDialog *dialog );

#endif 	    /* !CALLBACKS_H_ */
