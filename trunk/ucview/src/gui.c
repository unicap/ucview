/*
** gui.c
** 
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


#include "config.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include <unicap.h>

#include "ucview.h"
#include "gui.h"
#include "callbacks.h"
#include "ucview-about-dialog.h"
#include "icons.h"

static void showhide_toolbar_action( GtkAction *action, UCViewWindow *ucv );
static void showhide_sidebar_action( GtkAction *action, UCViewWindow *ucv );
static void scale_to_fit_action( GtkAction *action, UCViewWindow *ucv );
static void fullscreen_action( GtkAction *action, UCViewWindow *ucv );
static void leave_fullscreen_action( GtkAction *action, UCViewWindow *ucv );

static const gchar *ui_info = 
"<ui>"
" <menubar name='MenuBar'>"
"  <menu action='FileMenu'>"
"   <menuitem action='SaveImage'/>"
"   <separator/>"
"   <menuitem action='Quit'/>"
"  </menu>"
"  <menu action='DeviceMenu'>"
"   <menuitem action='ChangeDevice'/>"
"   <separator/>"
"   <menuitem action='Adjustments'/>"
"  </menu>"
"  <menu action='EditMenu'>"
"   <menuitem action='Copy'/>"
"   <separator/>"
"   <menuitem action='RecordVideo'/>"
"   <menuitem action='TimeLapse'/>"
"   <separator/>"
"   <menuitem action='Preferences'/>"
"  </menu>"
"  <menu action='ViewMenu'>"
"   <menuitem action='ScaleToFit'/>"
"   <menuitem action='Fullscreen'/>"
"   <separator/>"
"   <menuitem action='ViewToolbar'/>"
"   <menuitem action='ViewSidebar'/>"
"   <separator/>"
"   <menuitem action='Pause'/>"
"  </menu>"
"  <menu action='ImageMenu'>"
"   <menuitem action='ShowFPS'/>"
"  </menu>"
"  <menu action='HelpMenu'>"
"   <menuitem action='About'/>"
"  </menu>"
" </menubar>"
" <toolbar name='ToolBar'>"
"   <toolitem action='Pause'/>"
"   <separator/>"
"   <toolitem action='SaveImage'/>"
"   <toolitem action='RecordVideo'/>"
"   <toolitem action='TimeLapse'/>"
"   <separator/>"
"   <toolitem action='Fullscreen'/>"
"   <separator/>"
"   <toolitem action='Adjustments'/>"
"   <toolitem action='Preferences'/>"
" </toolbar>"
" <toolbar name='VToolBar'>"
"   <toolitem action='Pause'/>"
"   <separator/>"
"   <toolitem action='SaveImage'/>"
"   <toolitem action='RecordVideo'/>"
"   <separator/>"
"   <toolitem action='LeaveFullscreen'/>"
"   <separator/>"
"   <toolitem action='Adjustments'/>"
" </toolbar>"
"</ui>";

static GtkActionEntry entries[] =
{
   { "FileMenu", NULL, N_("_File") }, 
   { "DeviceMenu", NULL, N_("_Device") }, 
   { "ColorFormatMenu", NULL, N_("Change Video Format" ) }, 
   { "ResolutionMenu", NULL, N_("Change Resolution" ) },
   { "EditMenu", NULL, N_("_Edit") },
   { "ViewMenu", NULL, N_("_View") },
   { "ImageMenu", NULL, N_("_Tools") },
   { "HelpMenu", NULL, N_("_Help") },

   { "Quit", GTK_STOCK_QUIT, N_("_Quit"), NULL, N_("Quit"), G_CALLBACK( gtk_main_quit ) }, 
   { "Copy", GTK_STOCK_COPY, N_("_Copy"), "<control>C", N_("Copy"), G_CALLBACK( clipboard_copy_image_cb ) },
   { "Adjustments", "ucview-device-settings", N_("_Adjustments"), NULL, N_("Adjustments"), G_CALLBACK( display_property_dialog_cb ) }, 
   { "Preferences", "ucview-preferences", N_("_Preferences"), NULL, N_("Preferences"), G_CALLBACK( display_settings_dialog_cb ) }, 

   { "ChangeDevice", NULL, N_("_Change Device"), NULL, N_("Change Device"), G_CALLBACK( change_device_cb ) },
   { "SaveImage", "ucview-save-still-image", N_("_Save Image"), "<control>S", N_("Save Still Image"), G_CALLBACK( save_still_image_cb ) },

   { "Fullscreen", UCVIEW_STOCK_FULLSCREEN, N_("Fullscreen"), "F11", N_("Fullscreen"), G_CALLBACK( fullscreen_action ) },
   { "LeaveFullscreen", UCVIEW_STOCK_LEAVE_FULLSCREEN, N_("Leave Fullscreen"), NULL, N_("Leave Fullscreen"), G_CALLBACK( leave_fullscreen_action ) },


   { "About", GTK_STOCK_ABOUT, "About", NULL, "About", G_CALLBACK( ucview_about_dialog_show_action ) },
   
};
static const guint n_entries = G_N_ELEMENTS( entries );



static GtkToggleActionEntry toggle_entries[] = 
{

   { "Pause", UCVIEW_STOCK_PAUSE, N_("P_ause"), "<control>P", N_("Pause"), G_CALLBACK( pause_state_toggled_cb ) },
   { "RecordVideo", UCVIEW_STOCK_RECORD_VIDEO, N_("_Record Video"), "<control>R", N_("Record Video"), G_CALLBACK( record_toggled_cb ), FALSE },
   { "TimeLapse", UCVIEW_STOCK_RECORD_VIDEO, N_("_Time Lapse"), "<control>T", N_("Time Lapse"), G_CALLBACK( time_lapse_toggled_cb ), FALSE },
   { "PlayVideo", UCVIEW_STOCK_PLAY_VIDEO, N_("_Play Video"), "<control>V", N_("Play Video"), G_CALLBACK( play_toggled_cb ), FALSE },

   { "ShowFPS", NULL, N_("Show _FPS"), NULL, N_("Show FPS"), G_CALLBACK( fps_toggled_cb ) },
};
static const guint n_toggle_entries = G_N_ELEMENTS( toggle_entries );

static GtkToggleActionEntry scale_to_fit_toggle_entry = 
{
   "ScaleToFit", NULL, N_("_Scale To Fit"), NULL, N_("Scale To Fit"), G_CALLBACK( scale_to_fit_action ), TRUE
};

static GtkToggleActionEntry view_toolbar_toggle_entry = 
{ 
   "ViewToolbar", NULL, N_("_Toolbar"), NULL, N_("Toolbar"), G_CALLBACK( showhide_toolbar_action ), TRUE 
};

static GtkToggleActionEntry view_sidebar_toggle_entry = 
{
   "ViewSidebar", NULL, N_("Side_bar"), NULL, N_("Sidebar"), G_CALLBACK( showhide_sidebar_action ), TRUE
};



#if 0
static void activate_action( GtkAction *action, UCViewWindow *ucv )
{
   g_message( "Action '%s' activated\n", gtk_action_get_name( action ) );
}

static void activate_toggle_action( GtkAction *action, UCViewWindow *ucv )
{
   g_message( "ToggleAction '%s' %s\n", gtk_action_get_name( action ), gtk_toggle_action_get_active( GTK_TOGGLE_ACTION( action ) ) ? "activated" : "deactivated" );
}
#endif

static void showhide_toolbar_action( GtkAction *action, UCViewWindow *ucv )
{
   GtkWidget *widget;

   widget = gtk_ui_manager_get_widget( ucview_window_get_ui_manager( UCVIEW_WINDOW( ucv ) ), "/ToolBar" );
   g_assert( widget );
   
   if( gtk_toggle_action_get_active( GTK_TOGGLE_ACTION( action ) ) )
   {
      gtk_widget_show( widget );
   }
   else
   {
      gtk_widget_hide( widget );
   }   
   
   gconf_client_set_bool( ucv->client, UCVIEW_GCONF_DIR "/show_toolbar", gtk_toggle_action_get_active( GTK_TOGGLE_ACTION( action ) ), NULL );
}

static void showhide_sidebar_action( GtkAction *action, UCViewWindow *ucv )
{
   GtkWidget *widget;
   

   widget = glade_xml_get_widget( ucv->glade, "ucview_sidebar_box" );

   if( gtk_toggle_action_get_active( GTK_TOGGLE_ACTION( action ) ) )
   {
      gtk_widget_show( widget );
   }
   else
   {
      gtk_widget_hide( widget );
   }   
   
   gconf_client_set_bool( ucv->client, UCVIEW_GCONF_DIR "/show_sidebar", gtk_toggle_action_get_active( GTK_TOGGLE_ACTION( action ) ), NULL );
}



static void scale_to_fit_action( GtkAction *action, UCViewWindow *ucv )
{
   g_object_set( ucv->display, "scale-to-fit", gtk_toggle_action_get_active( GTK_TOGGLE_ACTION( action ) ), NULL );
   gconf_client_set_bool( ucv->client, UCVIEW_GCONF_DIR "/scale_to_fit", gtk_toggle_action_get_active( GTK_TOGGLE_ACTION( action ) ), NULL );
}

static void fullscreen_action( GtkAction *action, UCViewWindow *ucv )
{
   if( !(gdk_window_get_state( GTK_WIDGET( ucv )->window ) & GDK_WINDOW_STATE_FULLSCREEN ) )
   {
      ucview_window_set_fullscreen( ucv, TRUE );
      gconf_client_set_bool( ucv->client, UCVIEW_GCONF_DIR "/fullscreen", TRUE, NULL );
   }
   else
   {
      ucview_window_set_fullscreen( ucv, FALSE );
      gconf_client_set_bool( ucv->client, UCVIEW_GCONF_DIR "/fullscreen", FALSE, NULL );
   }      
}

static void leave_fullscreen_action( GtkAction *action, UCViewWindow *ucv )
{
   ucview_window_set_fullscreen( ucv, FALSE );
   gconf_client_set_bool( ucv->client, UCVIEW_GCONF_DIR "/fullscreen", FALSE, NULL );
      
}


GtkUIManager *gui_create_manager( UCViewWindow *ucv )
{
   GtkUIManager *ui;
   GtkActionGroup *actions;
   GError *error = NULL;   
   
   view_toolbar_toggle_entry.is_active = gconf_client_get_bool( ucv->client, UCVIEW_GCONF_DIR "/show_toolbar", NULL );
   scale_to_fit_toggle_entry.is_active = gconf_client_get_bool( ucv->client, UCVIEW_GCONF_DIR "/scale_to_fit", NULL );

   actions = gtk_action_group_new( "Actions" );
   gtk_action_group_set_translation_domain( actions, GETTEXT_PACKAGE );
   gtk_action_group_add_actions( actions, entries, n_entries, ucv );
   gtk_action_group_add_toggle_actions( actions, toggle_entries, n_toggle_entries, ucv );
   gtk_action_group_add_toggle_actions( actions, &view_toolbar_toggle_entry, 1, ucv );
   gtk_action_group_add_toggle_actions( actions, &view_sidebar_toggle_entry, 1, ucv );
   gtk_action_group_add_toggle_actions( actions, &scale_to_fit_toggle_entry, 1, ucv );
   
   
   ui = gtk_ui_manager_new();
   gtk_ui_manager_insert_action_group( ui, actions, 0 );
   g_object_unref( actions );
   
   if( !gtk_ui_manager_add_ui_from_string( ui, ui_info, -1, &error ) )
   {
      g_error( "Building menus failed: %s\n", error->message );
      g_error_free( error );
   }

   

   return ui;
}

/* static void gui_create_format_menu( UCViewWindow *ucv ) */
/* { */
   
/* } */
