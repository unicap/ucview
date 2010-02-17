/*
** device_dialog.h
** 
** Made by Arne Caspari
** Login   <arne@arne-laptop>
** 
** Started on  Wed Nov 15 07:19:25 2006 Arne Caspari
** Last update Wed Feb  7 10:40:02 2007 Arne Caspari
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

#ifndef   	DEVICE_DIALOG_H_
# define   	DEVICE_DIALOG_H_

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <unicap.h>






GType ucview_device_dialog_get_type( void );
GtkWidget *ucview_device_dialog_new( unicap_device_t *default_device );

#define UCVIEW_DEVICE_DIALOG_TYPE            (ucview_device_dialog_get_type())
#define UCVIEW_DEVICE_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), UCVIEW_DEVICE_DIALOG_TYPE, UCViewDeviceDialog))
#define UCVIEW_DEVICE_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), UCVIEW_DEVICE_DIALOG_TYPE, UCViewDeviceDialogClass))
#define IS_UCVIEW_DEVICE_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), UCVIEW_DEVICE_DIALOG_TYPE))
#define IS_UCVIEW_DEVICE_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), UCVIEW_DEVICE_DIALOG_TYPE))
#define UCVIEW_DEVICE_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), UCVIEW_DEVICE_DIALOG_TYPE, UCViewDeviceDialogClass))


typedef struct _UCViewDeviceDialog UCViewDeviceDialog;
typedef struct _UCViewDeviceDialogClass UCViewDeviceDialogClass;

struct _UCViewDeviceDialog
{
   GtkWindow window;
   GtkWidget *device_selection;
   GtkWidget *format_selection;
   GtkWidget *ok_button;
   GtkWidget *rescan_button;
   GtkWidget *check_button;
   gboolean delete;
   gboolean restore_device;
   
   unicap_handle_t handle;
   unicap_format_t format;
};

struct _UCViewDeviceDialogClass
{
      GtkWindowClass parent_class;
};


/* device_dialog_t *create_device_dialog( gboolean include_fmt_selection,  */
/* 				       unicap_device_t *default_device ); */

unicap_handle_t ucview_device_dialog_run( UCViewDeviceDialog *dlg );
unicap_handle_t ucview_device_dialog_get_handle( UCViewDeviceDialog *dlg );
void ucview_device_dialog_get_format( UCViewDeviceDialog *dlg, unicap_format_t *format );
void ucview_device_dialog_select_device( UCViewDeviceDialog *dlg, unicap_device_t *device );

#endif 	    /* !DEVICE_DIALOG_H_ */
