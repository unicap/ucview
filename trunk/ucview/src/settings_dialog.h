/*
** settings_dialog.h
** 
** Made by Arne Caspari
** Login   <arne@arne-laptop>
** 
** Started on  Fri Feb 16 17:15:01 2007 Arne Caspari
** Last update Mon Feb 19 18:20:46 2007 Arne Caspari
*/

#ifndef   	SETTINGS_DIALOG_H_
# define   	SETTINGS_DIALOG_H_

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtkwindow.h>
#include "ucview-window.h"

G_BEGIN_DECLS

#define SETTINGS_DIALOG_TYPE            (settings_dialog_get_type())
#define SETTINGS_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), SETTINGS_DIALOG_TYPE, SettingsDialog))
#define SETTINGS_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), SETTINGS_DIALOG_TYPE, SettingsDialogClass))
#define IS_SETTINGS_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SETTINGS_DIALOG_TYPE))
#define IS_SETTINGS_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SETTINGS_DIALOG_TYPE))


typedef struct _SettingsDialog        SettingsDialog;
typedef struct _SettingsDialogClass   SettingsDialogClass;
typedef struct _SettingsDialogPrivate SettingsDialogPrivate;

struct _SettingsDialog
{
   GtkWindow window;
   GtkWidget *video_file_button;
   GtkWidget *still_image_file_button;
   GtkWidget *plugin_tv;
   GtkWidget *about_plugin_button;
   GtkWidget *configure_plugin_button;
   GtkWidget *notebook;
   GtkWidget *image_file_types_combo;
   
      
   GConfClient *client;

   UCViewWindow *ucv_window;

   SettingsDialogPrivate *priv;
};

struct _SettingsDialogClass
{
      GtkWindowClass parent_class;

      void (* unicapgtk_property_dialog) (SettingsDialog *dlg);
      
};

GType              settings_dialog_get_type        ( void );
GtkWidget*         settings_dialog_new             ( );
void               settings_dialog_update_plugins  ( SettingsDialog *dlg, UCViewWindow *window );

G_END_DECLS


#endif 	    /* !SETTINGS_DIALOG_H_ */
