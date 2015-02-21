/*
** settings_dialog.c
** 
** Made by (Arne Caspari)
** Login   <arne@arne-laptop>
** 
** Started on  Fri Feb 16 17:13:05 2007 Arne Caspari
** Last update Mon Feb 19 22:16:40 2007 Arne Caspari
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

#include <glib.h>
#include <glib/gstdio.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gconf/gconf-client.h>
#include <unicap.h>
#include <ucil.h>
#include <string.h>

#include "settings_dialog.h"
#include "ucview.h"
#include "plugin_loader.h"
#include "utils.h"
#include "ucview-time-selection.h"


static void settings_dialog_class_init          (SettingsDialogClass *klass);
static void settings_dialog_init                (SettingsDialog      *dlg);
static void settings_dialog_set_property        ( GObject       *object, 
						  guint          property_id, 
						  const GValue  *value, 
						  GParamSpec    *pspec );
static void settings_dialog_get_property        ( GObject       *object, 
						  guint          property_id, 
						  GValue        *value, 
						  GParamSpec    *pspec );

static void enable_plugin_toggled_cb( GtkCellRenderer *cell, 
				       gchar *path_str, 
				       GtkTreeView *tv );

static void button_toggled_cb( GtkToggleButton *toggle, gchar *key );
static void activate_toggled_cb( GtkToggleButton *toggle, GtkWidget *widget );
static void audio_device_changed_cb( GtkComboBox *combo_box, GConfClient *client );
static void audio_bitrate_changed_cb( GtkComboBox *combo_box, GConfClient *client );
static void combo_box_text_changed_cb( GtkComboBox *combo_box, gchar *gconf_path );
static void auto_stop_recording_toggled_cb( GtkToggleButton *togglebutton, SettingsDialog *dlg );


G_DEFINE_TYPE( SettingsDialog, settings_dialog, GTK_TYPE_WINDOW );

#define SETTINGS_DIALOG_GET_PRIVATE( obj )( G_TYPE_INSTANCE_GET_PRIVATE( ( obj ), SETTINGS_DIALOG_TYPE, SettingsDialogPrivate ) )

struct _SettingsDialogPrivate
{
      unicap_format_t format;
};

enum
{
   PROP_0 = 0,
   PROP_VIDEO_FILENAME,
   PROP_STILL_IMAGE_FILENAME,
   PROP_FORMAT, 
};

enum
{
   COLUMN_ENABLE = 0, 
   COLUMN_NAME, 
   COLUMN_DATA, 
   COLUMN_UCVIEW, 
};

struct bitrate
{
      gchar *label;
      gint value;
};


static struct bitrate audio_bitrates[] =
{
   { "64 kbps", 64000 },
   { "96 kbps", 96000 }, 
   { "128 kbps", 128000 },
   { "196 kbps", 196000 }, 
   { "256 kbps", 256000 },
   { NULL, 0 }
};

static void time_lapse_frames_changed_cb( GtkSpinButton *spin, SettingsDialog *dlg )
{
   gconf_client_set_int( dlg->client, UCVIEW_GCONF_DIR "/time_lapse_frames", gtk_spin_button_get_value_as_int( spin ), NULL );
}

static void auto_stop_recording_toggled_cb( GtkToggleButton *togglebutton, SettingsDialog *dlg )
{
   gconf_client_set_bool( dlg->client, UCVIEW_GCONF_DIR "/time_lapse_auto_stop", gtk_toggle_button_get_active( togglebutton ), NULL );
   
   gtk_widget_set_sensitive( GTK_WIDGET( g_object_get_data( G_OBJECT( togglebutton ), "vbox" ) ), gtk_toggle_button_get_active( togglebutton ) );   
}

static void stop_condition_toggled_cb( GtkToggleButton *button, guint condition )
{
   GConfClient *client;

   if( gtk_toggle_button_get_active( button ) )
   {
      client = gconf_client_get_default();
      gconf_client_set_int( client, UCVIEW_GCONF_DIR "/time_lapse_stop_condition", condition, NULL );
      g_object_unref( client );
   }
}
   
static void time_changed_cb( UCViewTimeSelection *time_selection, guint time, gchar *key )
{
   GConfClient *client;
   gchar *gconf_path;
   
   client = gconf_client_get_default();
   gconf_path = g_strconcat( UCVIEW_GCONF_DIR, "/", key, NULL );
   
   gconf_client_set_int( client, gconf_path, (gint)time, NULL );
   
   g_free( gconf_path );
   g_object_unref( client );
}



static void file_activated_cb( GtkFileChooser *chooser, gchar *gconf_key )
{
   GConfClient *client;
   gchar *gconf_path;
   
   client = g_object_get_data( G_OBJECT( chooser ), "client" );
   g_assert( client );

   gconf_path = g_strconcat( UCVIEW_GCONF_DIR, "/", gconf_key, NULL );

   gconf_client_set_string( client, gconf_path, gtk_file_chooser_get_filename( chooser ), NULL );

   g_free( gconf_path );
}

static void video_bitrate_changed_cb( GtkSpinButton *spinner, SettingsDialog *dlg )
{
   gchar *gconf_path;

   gconf_path = g_strconcat( UCVIEW_GCONF_DIR, "/", "video_theora_bitrate", NULL );
   gconf_client_set_int( dlg->client, gconf_path, gtk_spin_button_get_value( spinner ), NULL );
   g_free( gconf_path );
}

static void video_quality_changed_cb( GtkSpinButton *spinner, SettingsDialog *dlg )
{
   gchar *gconf_path;

   gconf_path = g_strconcat( UCVIEW_GCONF_DIR, "/", "video_theora_quality", NULL );
   gconf_client_set_int( dlg->client, gconf_path, gtk_spin_button_get_value( spinner ), NULL );
   g_free( gconf_path );
}

static void video_frame_rate_changed_cb( GtkSpinButton *spinner, SettingsDialog *dlg )
{
   gchar *gconf_path;

   gconf_path = g_strconcat( UCVIEW_GCONF_DIR, "/", "video_frame_rate", NULL );
   gconf_client_set_float( dlg->client, gconf_path, gtk_spin_button_get_value( spinner ), NULL );
   g_free( gconf_path );
}

static void theora_combo_changed_cb( GtkComboBox *combo, SettingsDialog *dlg )
{
   gchar *gconf_path;
   int index;

   index = gtk_combo_box_get_active( combo );

   gconf_path = g_strconcat( UCVIEW_GCONF_DIR, "/", "video_theora_combo", NULL );
   gconf_client_set_int( dlg->client, gconf_path, index, NULL );
   g_free( gconf_path );
}

static void combo_set_video_bitrate_cb( GtkComboBox *combo, GtkSpinButton *spinner )
{
   int index;

   index = gtk_combo_box_get_active( combo );

   switch( index )
   {
      case 0:
	 // High Quality
	 gtk_spin_button_set_value( spinner, 8000 );
	 gtk_widget_set_sensitive( GTK_WIDGET( spinner ), FALSE );
	 break;

      case 1:
	 // Medium Quality
	 gtk_spin_button_set_value( spinner, 800 );
	 gtk_widget_set_sensitive( GTK_WIDGET( spinner ), FALSE );
	 break;

      case 2:
	 // Low Quality
	 gtk_spin_button_set_value( spinner, 300 );
	 gtk_widget_set_sensitive( GTK_WIDGET( spinner ), FALSE );
	 break;
	 
      default:
	 gtk_widget_set_sensitive( GTK_WIDGET( spinner ), TRUE );
	 break;
   }
}

static void combo_set_video_quality_cb( GtkComboBox *combo, GtkSpinButton *spinner )
{
   int index;

   index = gtk_combo_box_get_active( combo );

   switch( index )
   {
      case 0:
	 // High Quality
	 gtk_spin_button_set_value( spinner, 63 );
	 gtk_widget_set_sensitive( GTK_WIDGET( spinner ), FALSE );
	 break;

      case 1:
	 // Medium Quality
	 gtk_spin_button_set_value( spinner, 16 );
	 gtk_widget_set_sensitive( GTK_WIDGET( spinner ), FALSE );
	 break;

      case 2:
	 // Low Quality
	 gtk_spin_button_set_value( spinner, 8 );
	 gtk_widget_set_sensitive( GTK_WIDGET( spinner ), FALSE );
	 break;
	 
      default:
	 gtk_widget_set_sensitive( GTK_WIDGET( spinner ), TRUE );
	 break;
   }
}

static void combo_set_frame_rate_cb( GtkComboBox *combo, GtkSpinButton *spinner )
{
   int index;

   index = gtk_combo_box_get_active( combo );

   switch( index )
   {
      case 0:
	 // High Quality
	 gtk_spin_button_set_value( spinner, 30 );
	 gtk_widget_set_sensitive( GTK_WIDGET( spinner ), FALSE );
	 break;

      case 1:
	 // Medium Quality
	 gtk_spin_button_set_value( spinner, 30 );
	 gtk_widget_set_sensitive( GTK_WIDGET( spinner ), FALSE );
	 break;

      case 2:
	 // Low Quality
	 gtk_spin_button_set_value( spinner, 30 );
	 gtk_widget_set_sensitive( GTK_WIDGET( spinner ), FALSE );
	 break;
	 
      default:
	 gtk_widget_set_sensitive( GTK_WIDGET( spinner ), TRUE );
	 break;
   }
}

static void button_toggled_cb( GtkToggleButton *toggle, gchar *key )
{
   GConfClient *client;
   client = gconf_client_get_default();

   gconf_client_set_bool( client, key, gtk_toggle_button_get_active( toggle ), NULL );
   
   g_object_unref( client );
}

static void activate_toggled_cb( GtkToggleButton *toggle, GtkWidget *widget )
{
   gtk_widget_set_sensitive( widget, gtk_toggle_button_get_active( toggle ) );
}

static void audio_device_changed_cb( GtkComboBox *combo_box, GConfClient *client )
{
   gchar *text;
   text = gtk_combo_box_get_active_text( combo_box );
   if (text)
	   gconf_client_set_string( client, UCVIEW_GCONF_DIR "/alsa_card", text, NULL );
}

static void audio_bitrate_changed_cb( GtkComboBox *combo_box, GConfClient *client )
{
   gconf_client_set_int( client, UCVIEW_GCONF_DIR "/audio_bitrate", 
			 audio_bitrates[gtk_combo_box_get_active( combo_box )].value, NULL );
}

static void combo_box_text_changed_cb( GtkComboBox *combo_box, gchar *gconf_path )
{
   GConfClient *client = gconf_client_get_default();
   gchar *text = gtk_combo_box_get_active_text( combo_box );
   if( text == NULL ){
      gtk_combo_box_set_active( combo_box, 0 );
      return;
   }
   gconf_client_set_string( client, gconf_path, text, NULL );
   g_object_unref( client );
}

static void combo_box_model_changed_cb( GtkComboBox *combo, gint col )
{
   GConfClient *client = gconf_client_get_default();
   gchar *path;
   GtkTreeIter iter;
   GtkTreeModel *model = gtk_combo_box_get_model( combo );

   if( gtk_combo_box_get_active_iter( combo, &iter ) )
   {
      GType type;

      path = g_object_get_data( G_OBJECT( combo ), "gconf_path" );
      g_assert( path );

      type = gtk_tree_model_get_column_type( model, col );

      switch( type )
      {
	 case G_TYPE_INT:
	 case G_TYPE_UINT:
	 {
	    gint val;

	    gtk_tree_model_get( model, &iter, col, &val, -1 );
	    gconf_client_set_int( client, path, val, NULL );
	 }
	 break;
	 
	 case G_TYPE_STRING:
	 {
	    gchar *string;
	    gtk_tree_model_get( model, &iter, col, &string, -1 );
	    if( string )
	    {
	       gconf_client_set_string( client, path, string, NULL );
	       g_free( string );
	    }
	 }
	 break;
      }
   }
   g_object_unref( client );
}   


static void settings_dialog_class_init( SettingsDialogClass *klass )
{
   GObjectClass *object_class = G_OBJECT_CLASS( klass );
/*    GtkObjectClass *gtk_object_class = GTK_OBJECT_CLASS( klass ); */

   object_class->set_property = settings_dialog_set_property;
   object_class->get_property = settings_dialog_get_property;

   g_type_class_add_private( object_class, sizeof( SettingsDialogPrivate ) );

   g_object_class_install_property (object_class,
				    PROP_VIDEO_FILENAME,
				    g_param_spec_string ("video-filename",
							 NULL, NULL,
							 FALSE,
							 G_PARAM_READWRITE));
   g_object_class_install_property (object_class,
				    PROP_STILL_IMAGE_FILENAME,
				    g_param_spec_string ("still-image-filename",
							 NULL, NULL,
							 FALSE,
							 G_PARAM_READWRITE));
   

}

static GtkWidget *create_rawavi_settings_vbox( SettingsDialog *dlg )
{
   GtkWidget *frame_vbox;
   GtkWidget *hbox;
   GtkWidget *label;
   GtkWidget *combo;
   GtkListStore *store;
   GtkTreeIter iter;
   GtkCellRenderer *renderer;
   unsigned int colorformat;
   int sel = 0;
   
   frame_vbox = gtk_vbox_new( FALSE, 0 );

   hbox = gtk_hbox_new( FALSE, 0 );
   gtk_box_pack_start( GTK_BOX( frame_vbox ), hbox, FALSE, FALSE, 12 );

   label = gtk_label_new( _("Colorformat: ") );
   gtk_box_pack_start( GTK_BOX( hbox ), label, FALSE, FALSE, 12 );

   store = gtk_list_store_new( 2, G_TYPE_STRING, G_TYPE_UINT );   
   gtk_list_store_append( store, &iter );
   gtk_list_store_set( store, &iter, 0, _("From Capture Device"), 1, 0, -1 );
   gtk_list_store_append( store, &iter );
   gtk_list_store_set( store, &iter, 0, _("UYVY"), 1, UCIL_FOURCC( 'U', 'Y', 'V', 'Y' ), -1 );
   gtk_list_store_append( store, &iter );
   gtk_list_store_set( store, &iter, 0, _("Y800"), 1, UCIL_FOURCC( 'Y', '8', '0', '0' ), -1 );
   gtk_list_store_append( store, &iter );
   gtk_list_store_set( store, &iter, 0, _("RGB 24"), 1, UCIL_FOURCC( 'R', 'G', 'B', '3' ), -1 );
   combo = gtk_combo_box_new_with_model( GTK_TREE_MODEL( store ) );
   gtk_box_pack_start( GTK_BOX( hbox ), combo, FALSE, FALSE, 12 );
   renderer = gtk_cell_renderer_text_new();
   gtk_cell_layout_pack_start( GTK_CELL_LAYOUT( combo ), renderer, FALSE );
   gtk_cell_layout_set_attributes( GTK_CELL_LAYOUT( combo ), renderer, "text", 0, NULL );
   colorformat = (unsigned int ) gconf_client_get_int( dlg->client, UCVIEW_GCONF_DIR "/rawavi_colorformat", NULL );
   switch( colorformat )
   {
      case 0:
	 sel = 0;
	 break;
	 
      case UCIL_FOURCC( 'U', 'Y', 'V', 'Y' ):
	 sel = 1;
	 break;
	 
      case UCIL_FOURCC( 'Y', '8', '0', '0' ):
	 sel = 2;
	 break;

      case UCIL_FOURCC( 'R', 'G', 'B', '3' ):
	 sel = 3;
	 break;
   }
   g_object_set_data( G_OBJECT( combo ), "gconf_path", UCVIEW_GCONF_DIR "/rawavi_colorformat" );
   g_signal_connect( combo, "changed", (GCallback)combo_box_model_changed_cb, (gpointer)1 );
   gtk_combo_box_set_active( GTK_COMBO_BOX( combo ), sel );
   
   gtk_widget_show_all( frame_vbox );

   return frame_vbox;
}

static GtkWidget *create_theora_settings_vbox( SettingsDialog *dlg )
{
   GtkWidget *label;
   GtkWidget *vbox;
   GtkWidget *hbox;   
   GtkWidget *frame_vbox;
   GtkWidget *combo;
   GtkSizeGroup *size_group;
   int combo_sel;
   GtkListStore *store;
   GtkTreeIter iter;
   GtkCellRenderer *renderer;
   
   
   size_group = gtk_size_group_new( GTK_SIZE_GROUP_HORIZONTAL );

   frame_vbox = gtk_vbox_new( FALSE, 0 );
   hbox = gtk_hbox_new( FALSE, 0 );
   gtk_box_pack_start( GTK_BOX( frame_vbox ), hbox, FALSE, FALSE, 8 );
   
   label = gtk_label_new( _("Video Size:") );
   gtk_misc_set_alignment( GTK_MISC( label ), 0.0, 0.5 );
   gtk_size_group_add_widget( size_group, label );
   gtk_box_pack_start( GTK_BOX( hbox ), label, FALSE, FALSE, 12 );   

   store = gtk_list_store_new( 2, G_TYPE_STRING, G_TYPE_INT );
   gtk_list_store_append( store, &iter );
   gtk_list_store_set( store, &iter, 0, _("Original"), 1, 1, -1 );
   gtk_list_store_append( store, &iter );
   gtk_list_store_set( store, &iter, 0, _("Half Size"), 1, 2, -1 );
   gtk_list_store_append( store, &iter );
   gtk_list_store_set( store, &iter, 0, _("Quarter Size"), 1, 4, -1 );
   combo = gtk_combo_box_new_with_model( GTK_TREE_MODEL( store ) );
   gtk_box_pack_start( GTK_BOX( hbox ), combo, FALSE, FALSE, 12 );
   renderer = gtk_cell_renderer_text_new();
   gtk_cell_layout_pack_start( GTK_CELL_LAYOUT( combo ), renderer, FALSE );
   gtk_cell_layout_set_attributes( GTK_CELL_LAYOUT( combo ), renderer, "text", 0, NULL );
   combo_sel = gconf_client_get_int( dlg->client, UCVIEW_GCONF_DIR "/video_downsize", NULL );
   switch( combo_sel )
   {
      case 2:
	 combo_sel = 1; break;
      case 4:
	 combo_sel = 2; break;
      default: 
	 combo_sel = 0; break;
   }
   g_object_set_data( G_OBJECT( combo ), "gconf_path", UCVIEW_GCONF_DIR "/video_downsize" );
   gtk_combo_box_set_active( GTK_COMBO_BOX( combo ), combo_sel );
   g_signal_connect( combo, "changed", (GCallback)combo_box_model_changed_cb, (gpointer)1 );

   hbox = gtk_hbox_new( FALSE, 0 );
   gtk_box_pack_start( GTK_BOX( frame_vbox ), hbox, FALSE, FALSE, 8 );

   label = gtk_label_new( "Compression Quality:" );
   gtk_misc_set_alignment( GTK_MISC( label ), 0.0, 0.5 );
   gtk_size_group_add_widget( size_group, label );
   gtk_box_pack_start( GTK_BOX( hbox ), label, FALSE, FALSE, 12 );


   combo = gtk_combo_box_new_text();
   gtk_combo_box_append_text( GTK_COMBO_BOX( combo ), _("High Quality / Large Files") );
   gtk_combo_box_append_text( GTK_COMBO_BOX( combo ), _("Medium Quality") );
   gtk_combo_box_append_text( GTK_COMBO_BOX( combo ), _("Low Quality / Small Files") );
   gtk_combo_box_append_text( GTK_COMBO_BOX( combo ), _("Custom") );   
   gtk_box_pack_start( GTK_BOX( hbox ), combo, TRUE, FALSE, 12 );
   {
      combo_sel = gconf_client_get_int( dlg->client, UCVIEW_GCONF_DIR "/video_theora_combo", NULL );

      gtk_combo_box_set_active( GTK_COMBO_BOX( combo ), combo_sel );
      g_signal_connect( G_OBJECT( combo ), "changed", G_CALLBACK( theora_combo_changed_cb ), dlg );
   }
   
   hbox = gtk_hbox_new( FALSE, 0 );
   gtk_box_pack_start( GTK_BOX( frame_vbox ), hbox, TRUE, TRUE, 0 );
   vbox = gtk_vbox_new( FALSE, 0 );
   gtk_box_pack_start( GTK_BOX( hbox ), vbox, TRUE, TRUE, 12 );

   hbox = gtk_hbox_new( FALSE, 0 );
   gtk_box_pack_start( GTK_BOX( vbox ), hbox, FALSE, FALSE, 8 );
   label = gtk_label_new( _("Target Bitrate: ") );
   gtk_misc_set_alignment( GTK_MISC( label ), 0.0, 0.5 );
   gtk_size_group_add_widget( size_group, label );
   gtk_box_pack_start( GTK_BOX( hbox ), label, FALSE, FALSE, 12 );
   {
      GtkWidget *spinner;
      GtkAdjustment *adj;
      gint default_value;
      
      default_value = gconf_client_get_int( dlg->client, UCVIEW_GCONF_DIR "/video_theora_bitrate", NULL );
      
      adj = GTK_ADJUSTMENT( gtk_adjustment_new( default_value, 100.0, 10000.0, 50.0, 250.0, 0.0 ) );
      if( ( default_value < 100 ) || ( default_value > 10000 ) )
      {
	 gtk_adjustment_set_value( adj, 800.0 );
      }
      spinner = gtk_spin_button_new( adj, 50.0, 0 );
      g_signal_connect( G_OBJECT( spinner ), "value-changed", G_CALLBACK( video_bitrate_changed_cb ), dlg );
      
      gtk_box_pack_start( GTK_BOX( hbox ), spinner, FALSE, FALSE, 0 );

      g_signal_connect( G_OBJECT( combo ), "changed", G_CALLBACK( combo_set_video_bitrate_cb ), spinner );

      if( combo_sel != 3 )
      {
	 gtk_widget_set_sensitive( spinner, FALSE );
      }
      
   }
   label = gtk_label_new( _("kbps") );
   gtk_box_pack_start( GTK_BOX( hbox ), label, FALSE, FALSE, 12 );

   hbox = gtk_hbox_new( FALSE, 0 );
   gtk_box_pack_start( GTK_BOX( vbox ), hbox, FALSE, FALSE, 8 );
   
   label = gtk_label_new( _("Encoding quality: ") );
   gtk_misc_set_alignment( GTK_MISC( label ), 0.0, 0.5 );
   gtk_size_group_add_widget( size_group, label );
   gtk_box_pack_start( GTK_BOX( hbox ), label, FALSE, FALSE, 12 );
   
   {
      GtkWidget *spinner;
      GtkAdjustment *adj;
      gint default_value;

      default_value = gconf_client_get_int( dlg->client, UCVIEW_GCONF_DIR "/video_theora_quality", NULL );
      
      adj = GTK_ADJUSTMENT( gtk_adjustment_new( default_value, 0.0, 63.0, 1.0, 5.0, 0.0 ) );
      if( ( default_value < 0 ) || ( default_value > 63 ) )
      {
	 gtk_adjustment_set_value( adj, 16.0 );
      }
      spinner = gtk_spin_button_new( adj, 1.0, 0 );
      
      g_signal_connect( G_OBJECT( spinner ), "value-changed", G_CALLBACK( video_quality_changed_cb ), dlg );
      gtk_box_pack_start( GTK_BOX( hbox ), spinner, FALSE, FALSE, 0 );

      g_signal_connect( G_OBJECT( combo ), "changed", G_CALLBACK( combo_set_video_quality_cb ), spinner );

      if( combo_sel != 3 )
      {
	 gtk_widget_set_sensitive( spinner, FALSE );
      }

   }   

   hbox = gtk_hbox_new( FALSE, 0 );
   gtk_box_pack_start( GTK_BOX( vbox ), hbox, FALSE, FALSE, 8 );
   
   label = gtk_label_new( _("Encoding frame rate: ") );
   gtk_misc_set_alignment( GTK_MISC( label ), 0.0, 0.5 );
   gtk_size_group_add_widget( size_group, label );
   gtk_box_pack_start( GTK_BOX( hbox ), label, FALSE, FALSE, 12 );
   
   {
      GtkWidget *spinner;
      GtkAdjustment *adj;
      gdouble default_value;

      default_value = gconf_client_get_float( dlg->client, UCVIEW_GCONF_DIR "/video_frame_rate", NULL );
      
      adj = GTK_ADJUSTMENT( gtk_adjustment_new( default_value, 0.0, 120.0, 1.0, 5.0, 0.0 ) );
      if( ( default_value <= 0.0 ) || ( default_value > 120.0 ) )
      {
	 gtk_adjustment_set_value( adj, 30.0 );
      }
      spinner = gtk_spin_button_new( adj, 1.0, 0 );
      
      g_signal_connect( G_OBJECT( spinner ), "value-changed", G_CALLBACK( video_frame_rate_changed_cb ), dlg );
      gtk_box_pack_start( GTK_BOX( hbox ), spinner, FALSE, FALSE, 0 );

      g_signal_connect( G_OBJECT( combo ), "changed", G_CALLBACK( combo_set_frame_rate_cb ), spinner );

      if( combo_sel != 3 )
      {
	 gtk_widget_set_sensitive( spinner, FALSE );
      }

   }   

   return frame_vbox;
}

static GtkWidget *create_audio_page( SettingsDialog *dlg )
{
   GtkWidget *vbox;
   GtkWidget *vbox2;
   GtkWidget *hbox;
   GtkWidget *label;
   GtkWidget *frame;
   GtkWidget *check_button;
   GtkWidget *enable_button;
   GtkWidget *widget;
   GList *list;
   GList *entry;
   int i, sel;
   gchar *audio_hw;

   vbox = gtk_vbox_new( FALSE, 0 );
   
   enable_button = gtk_check_button_new_with_label( _( "Enable audio recording" ) );
   gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( enable_button ), gconf_client_get_bool( dlg->client, UCVIEW_GCONF_DIR "/record_audio", NULL ) );
   g_signal_connect( enable_button, "toggled", (GCallback)button_toggled_cb, UCVIEW_GCONF_DIR "/record_audio" );
   hbox = gtk_hbox_new( FALSE, 0 );
   gtk_box_pack_start( GTK_BOX( vbox ), hbox, FALSE, FALSE, 12 );
   gtk_box_pack_start( GTK_BOX( hbox ), enable_button, FALSE, FALSE, 12 );

   hbox = gtk_hbox_new( FALSE, 0 );
   gtk_box_pack_start( GTK_BOX( vbox ), hbox, TRUE, TRUE, 0 );

   frame = gtk_frame_new( _("Audio Settings") );
   gtk_container_set_border_width( GTK_CONTAINER( frame ), 12 );
   gtk_box_pack_start( GTK_BOX( hbox ), frame, TRUE, TRUE, 0 );

   vbox2 = gtk_vbox_new( FALSE, 0 );
   gtk_container_add( GTK_CONTAINER( frame ), vbox2 );
   
   hbox = gtk_hbox_new( FALSE, 0 );
   gtk_box_pack_start( GTK_BOX( vbox2 ), hbox, FALSE, FALSE, 0 );
   
   check_button = gtk_check_button_new_with_label( _( "Delay audio encoding" ) );
   gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( check_button ), gconf_client_get_bool( dlg->client, UCVIEW_GCONF_DIR "/async_audio_encode", NULL ) );
   g_signal_connect( check_button, "toggled", (GCallback)button_toggled_cb, UCVIEW_GCONF_DIR "/async_audio_encode" );
   hbox = gtk_hbox_new( FALSE, 0 );
   gtk_box_pack_start( GTK_BOX( vbox2 ), hbox, FALSE, FALSE, 12 );
   gtk_box_pack_start( GTK_BOX( hbox ), check_button, FALSE, FALSE, 12 );

   g_signal_connect( enable_button, "toggled", (GCallback)activate_toggled_cb, frame );


   label = gtk_label_new( _( "Audio device: " ) );
   widget = gtk_combo_box_new_text();
   list = ucil_audio_list_cards();
   audio_hw = gconf_client_get_string( dlg->client, UCVIEW_GCONF_DIR "/alsa_card", NULL );
   sel = 0;
   for( entry = list, i = 0; entry; entry = entry->next, i++ )
   {
      gtk_combo_box_append_text( GTK_COMBO_BOX( widget ), (gchar*)entry->data );
      g_free( entry->data );
      if( audio_hw && !strcmp( entry->data, audio_hw ) )
      {
	 sel = i;
      }
   }
   g_list_free( list );
   hbox = gtk_hbox_new( FALSE, 0 );
   gtk_box_pack_start( GTK_BOX( vbox2 ), hbox, FALSE, FALSE, 12 );
   gtk_box_pack_start( GTK_BOX( hbox ), label, FALSE, FALSE, 12 );
   gtk_box_pack_start( GTK_BOX( hbox ), widget, FALSE, FALSE, 12 );
   g_signal_connect( widget, "changed", (GCallback)audio_device_changed_cb, dlg->client );
   gtk_combo_box_set_active( GTK_COMBO_BOX( widget ), sel );

   label = gtk_label_new( _( "Encoding bitrate: " ) );
   widget = gtk_combo_box_new_text();
   sel = 0;
   for( i = 0; audio_bitrates[i].label; i++ )
   {
      int bitrate;
      bitrate = gconf_client_get_int( dlg->client, UCVIEW_GCONF_DIR "/audio_bitrate", NULL );
      gtk_combo_box_append_text( GTK_COMBO_BOX( widget ), audio_bitrates[i].label );
      if( bitrate == audio_bitrates[i].value )
      {
	 sel = i;
      }
   }
   hbox = gtk_hbox_new( FALSE, 0 );
   gtk_box_pack_start( GTK_BOX( vbox2 ), hbox, FALSE, FALSE, 12 );
   gtk_box_pack_start( GTK_BOX( hbox ), label, FALSE, FALSE, 12 );
   gtk_box_pack_start( GTK_BOX( hbox ), widget, FALSE, FALSE, 12 );
   g_signal_connect( widget, "changed", (GCallback)audio_bitrate_changed_cb, dlg->client );
   gtk_combo_box_set_active( GTK_COMBO_BOX( widget ), sel );

   gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( enable_button ), gconf_client_get_bool( dlg->client, UCVIEW_GCONF_DIR "/record_audio", NULL ) );
   gtk_widget_set_sensitive( frame, gconf_client_get_bool( dlg->client, UCVIEW_GCONF_DIR "/record_audio", NULL ) );

   return vbox;
}

static void encoder_combo_changed_cb( GtkComboBox *combo, GtkWidget *frame )
{
   GtkWidget *frame_vbox;
   SettingsDialog *dlg = g_object_get_data( G_OBJECT( combo ), "dlg" );
   GtkWidget *child;
   
   child = gtk_bin_get_child( GTK_BIN( frame ) );
   if( child )
   {
      gtk_widget_destroy( child );
   }
   
   switch( gtk_combo_box_get_active( combo ) )
   {
      case 0:
	 // ogg/theora
	 frame_vbox = create_theora_settings_vbox( dlg );
	 gconf_client_set_string( dlg->client, UCVIEW_GCONF_DIR "/video_encoder", "ogg/theora", NULL );
	 break;

      case 1:
	 //rawavi
	 frame_vbox = create_rawavi_settings_vbox( dlg );
	 gconf_client_set_string( dlg->client, UCVIEW_GCONF_DIR "/video_encoder", "avi/raw", NULL );
	 break;
	 
      default:
	 return;
   } 
   gtk_widget_show_all( frame_vbox );
   
   gtk_container_add( GTK_CONTAINER( frame ), frame_vbox );
}



static GtkWidget *create_video_page( SettingsDialog *dlg )
{
   GtkWidget *vbox;
   GtkWidget *hbox;
   GtkWidget *label;
   GtkWidget *file_button;
   GtkWidget *frame;
   GtkWidget *combo;
   gchar *path;
   gchar *encoder;
   int active = 0;

   vbox = gtk_vbox_new( FALSE, 0 );
   
   hbox = gtk_hbox_new( FALSE, 0 );
   gtk_box_pack_start( GTK_BOX( vbox ), hbox, FALSE, FALSE, 12 );

   label = gtk_label_new( _("Video File Path: ") );
   gtk_box_pack_start( GTK_BOX( hbox ), label, FALSE, FALSE, 12 );
   file_button = gtk_file_chooser_button_new( _("Video File Path"), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER );
   gtk_box_pack_start( GTK_BOX( hbox ), file_button, TRUE, TRUE, 12 );
   dlg->video_file_button = file_button;
   if( ( path = gconf_client_get_string( dlg->client, UCVIEW_GCONF_DIR "/video_file_path", NULL ) ) && 
       g_file_test( path, G_FILE_TEST_IS_DIR ) && 
       ( g_access( path, W_OK ) == 0 ) )
   {
      gtk_file_chooser_set_current_folder( GTK_FILE_CHOOSER( file_button ), path );
   }
   else
   {
      gtk_file_chooser_set_current_folder( GTK_FILE_CHOOSER( file_button ), g_get_home_dir() );
   }
   
   g_object_set_data( G_OBJECT( file_button ), "client", dlg->client );
   g_signal_connect( G_OBJECT( file_button ), "selection-changed", G_CALLBACK( file_activated_cb ), "video_file_path" );

   hbox = gtk_hbox_new( FALSE, 0 );
   gtk_box_pack_start( GTK_BOX( vbox ), hbox, FALSE, FALSE, 12 );

   label = gtk_label_new( _("Video Encoding: ") );
   gtk_box_pack_start( GTK_BOX( hbox ), label, FALSE, FALSE, 12 );
   
   combo = gtk_combo_box_new_text( );
   gtk_box_pack_start( GTK_BOX( hbox ), combo, FALSE, FALSE, 12 );
   gtk_combo_box_append_text( GTK_COMBO_BOX( combo ), _("OGG/Theora") );
   if( ucil_get_video_file_extension( "avi/raw" ) != NULL )
   {
      gtk_combo_box_append_text( GTK_COMBO_BOX( combo ), _("Uncompressed AVI") );
   }
   

   hbox = gtk_hbox_new( FALSE, 0 );
   gtk_box_pack_start( GTK_BOX( vbox ), hbox, FALSE, FALSE, 0 );

   frame = gtk_frame_new( _("Encoder Settings") );
   gtk_container_set_border_width( GTK_CONTAINER( frame ), 12 );
   gtk_box_pack_start( GTK_BOX( hbox ), frame, FALSE, FALSE, 0 );

   g_object_set_data( G_OBJECT( combo ), "dlg", dlg );
   
   g_signal_connect( combo, "changed", (GCallback)encoder_combo_changed_cb, frame );
   encoder = gconf_client_get_string( dlg->client, UCVIEW_GCONF_DIR "/video_encoder", NULL );
   
   if( encoder && !strcmp( encoder, "avi/raw" ) )
   {
      active = 1;
   }
   
   gtk_combo_box_set_active( GTK_COMBO_BOX( combo ), active );
   

   return vbox;
}

static void enable_plugin_toggled_cb( GtkCellRenderer *cell, 
				       gchar *path_str, 
				       GtkTreeView *tv )
{
   GtkTreeIter  iter;
   GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
   UCViewPluginData *data;
   UCViewWindow *window;
   gboolean enable;
   GtkTreeModel *model = gtk_tree_view_get_model( tv );


   /* get toggled iter */
   gtk_tree_model_get_iter (model, &iter, path);
   gtk_tree_model_get (model, &iter, COLUMN_DATA, &data, COLUMN_ENABLE, &enable, COLUMN_UCVIEW, &window, -1);

   ucview_enable_plugin( window, data, !enable );
   
   /* set new value */
   gtk_list_store_set (GTK_LIST_STORE (model), &iter, COLUMN_ENABLE, data->enable, -1);
   
   /* clean up */
   gtk_tree_path_free (path);
}

static void about_plugin_clicked_cb( GtkButton *button, GtkTreeView *tv )
{
   GtkTreeModel *model;
   GtkTreeSelection *sel = gtk_tree_view_get_selection( tv );
   GtkTreeIter iter;   
   

   if( gtk_tree_selection_get_selected( sel, &model, &iter ) )
   {
      GtkWindow *window;
      UCViewPluginData *data;
      GtkWidget *dlg;

      window = GTK_WINDOW( gtk_widget_get_toplevel( GTK_WIDGET( button ) ) );
      
      gtk_tree_model_get( model, &iter, COLUMN_DATA, &data, -1 );
      
      dlg = g_object_new( GTK_TYPE_ABOUT_DIALOG,
			  "program_name", data->plugin_data->name,
			  "comments", data->plugin_data->description,
			  "authors", data->plugin_data->authors,
			  "copyright", data->plugin_data->copyright,
			  "license", data->plugin_data->license,
			  "version", data->plugin_data->version,
			  "website", data->plugin_data->website,
			  NULL );
      
      gtk_widget_show_all( dlg );

      g_signal_connect_swapped( dlg, "response", G_CALLBACK( gtk_widget_destroy ), dlg );
      g_signal_connect_swapped( window, "hide", G_CALLBACK( gtk_widget_destroy ), dlg );
   }
}

static void configure_plugin_clicked_cb( GtkButton *button, GtkTreeView *tv )
{
   GtkTreeModel *model;
   GtkTreeSelection *sel = gtk_tree_view_get_selection( tv );
   GtkTreeIter iter;   
   

   if( gtk_tree_selection_get_selected( sel, &model, &iter ) )
   {
      UCViewPluginData *data;
      UCViewPluginConfigureFunc func;

      gtk_tree_model_get( model, &iter, COLUMN_DATA, &data, -1 );

      if( g_module_symbol( data->module, "ucview_plugin_configure", (void*)&func ) )
      {
	 GtkWidget *window;
	 window = func( data->plugin_data );
	 if( window )
	 {
	    gtk_window_set_transient_for( GTK_WINDOW( window ), GTK_WINDOW( gtk_widget_get_toplevel( GTK_WIDGET( button ) ) ) );
	    gtk_window_present( GTK_WINDOW( window ) );
	 } 
      }
   }
}

static void plugin_tv_cursor_changed_cb( GtkTreeView *tv, SettingsDialog *dlg )
{
   GtkTreeSelection *sel = gtk_tree_view_get_selection( tv );
   GtkTreeModel *model;
   GtkTreeIter iter;
   
   if( gtk_tree_selection_get_selected( sel, &model, &iter ) )
   {
      UCViewPluginData *data;
      UCViewPluginConfigureFunc func;
      
      gtk_tree_model_get( model, &iter, COLUMN_DATA, &data, -1 );
      
      gtk_widget_set_sensitive( dlg->configure_plugin_button, g_module_symbol( data->module, "ucview_plugin_configure", (void*)&func ) );
   }

   gtk_widget_set_sensitive( dlg->about_plugin_button, TRUE );
}

static GtkTreeModel *create_plugin_model( UCViewWindow *ucv_window )
{
   GtkListStore *store;
   GList *e;
   
   store = gtk_list_store_new( 4, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_POINTER );
   
   for( e = ucv_window->modules; e; e = g_list_next( e ) )
   {
      GtkTreeIter iter;
      UCViewPluginData *data;
      gchar *text;
      
      
      data = (UCViewPluginData *) e->data;
      
      gtk_list_store_append( store, &iter );
      text = g_strconcat( "<b>", data->plugin_data->name,"</b>\n<i>", data->plugin_data->description, "</i>", NULL );
      gtk_list_store_set( store, &iter, 
			  COLUMN_ENABLE, data->enable, 
			  COLUMN_NAME, text, 
			  COLUMN_DATA, data, 
			  COLUMN_UCVIEW, ucv_window, 
			  -1 );
      g_free( text );
   }
   
   return GTK_TREE_MODEL( store );
}

static void add_plugin_columns( GtkTreeView *tv )
{
   GtkCellRenderer *renderer;
   GtkTreeViewColumn *column;
/*    GtkTreeModel *model = gtk_tree_view_get_model( tv ); */
   

   renderer = gtk_cell_renderer_toggle_new();
   g_signal_connect( renderer, "toggled", G_CALLBACK( enable_plugin_toggled_cb ), tv );   
   column = gtk_tree_view_column_new_with_attributes( _("Active"), renderer, "active", 0, NULL );
   gtk_tree_view_column_set_sizing( column, GTK_TREE_VIEW_COLUMN_FIXED );
   gtk_tree_view_column_set_fixed_width( column, 50 );
   gtk_tree_view_append_column( tv, column );
   
   renderer = gtk_cell_renderer_text_new();
   column = gtk_tree_view_column_new_with_attributes( _("Name"), renderer, "markup", 1, NULL );
   gtk_tree_view_append_column( tv, column );
}


static GtkWidget *create_plugin_page( SettingsDialog *dlg )
{
   GtkWidget *vbox;
   GtkWidget *hbox;

   vbox = gtk_vbox_new( FALSE, 0 );
   dlg->plugin_tv = gtk_tree_view_new( );
   add_plugin_columns( GTK_TREE_VIEW( dlg->plugin_tv ) );
   
   gtk_box_pack_start( GTK_BOX( vbox ), dlg->plugin_tv, FALSE, FALSE, 12 );

   hbox = gtk_hbox_new( FALSE, 0 );
   gtk_box_pack_start( GTK_BOX( vbox ), hbox, FALSE, FALSE, 12 );
   
   dlg->about_plugin_button = ucview_gtk_button_new_with_stock_icon( _("_About Plugin"), GTK_STOCK_ABOUT );
   g_signal_connect( dlg->about_plugin_button, "clicked", G_CALLBACK( about_plugin_clicked_cb ), dlg->plugin_tv );
   gtk_box_pack_start( GTK_BOX( hbox ), dlg->about_plugin_button, FALSE, FALSE, 12 );
   gtk_widget_set_sensitive( dlg->about_plugin_button, FALSE );
   
   dlg->configure_plugin_button = ucview_gtk_button_new_with_stock_icon( _("C_onfigure Plugin"), GTK_STOCK_PREFERENCES );
   g_signal_connect( dlg->configure_plugin_button, "clicked", G_CALLBACK( configure_plugin_clicked_cb ), dlg->plugin_tv );
   gtk_box_pack_start( GTK_BOX( hbox ), dlg->configure_plugin_button, FALSE, FALSE, 12 );
   g_signal_connect( dlg->plugin_tv, "cursor-changed", G_CALLBACK( plugin_tv_cursor_changed_cb ), dlg );
   
   return vbox;
}


static GtkWidget *create_image_page( SettingsDialog *dlg )
{
   GtkWidget *vbox;
   GtkWidget *hbox;
   GtkWidget *label;
   GtkWidget *file_button;
   GtkWidget *combo;
   gchar *path;

   vbox = gtk_vbox_new( FALSE, 0 );
   
   hbox = gtk_hbox_new( FALSE, 0 );
   gtk_box_pack_start( GTK_BOX( vbox ), hbox, FALSE, FALSE, 12 );

   label = gtk_label_new( _("Still Image File Path: ") );
   gtk_box_pack_start( GTK_BOX( hbox ), label, FALSE, FALSE, 12 );
   file_button = gtk_file_chooser_button_new( _("Still Image File Path"), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER );
   gtk_box_pack_start( GTK_BOX( hbox ), file_button, TRUE, TRUE, 12 );
   dlg->still_image_file_button = file_button;
   if( ( path = gconf_client_get_string( dlg->client, UCVIEW_GCONF_DIR "/still_image_file_path", NULL ) ) && 
       g_file_test( path, G_FILE_TEST_IS_DIR ) && 
       ( g_access( path, W_OK ) == 0 ) )
   {
      gtk_file_chooser_set_current_folder( GTK_FILE_CHOOSER( file_button ), path );
   }
   else
   {
      gtk_file_chooser_set_current_folder( GTK_FILE_CHOOSER( file_button ), g_get_home_dir() );
   }

   g_object_set_data( G_OBJECT( file_button ), "client", dlg->client );
   g_signal_connect( G_OBJECT( file_button ), "selection-changed", G_CALLBACK( file_activated_cb ), "still_image_file_path" );

   hbox = gtk_hbox_new( FALSE, 0 );
   gtk_box_pack_start( GTK_BOX( vbox ), hbox, FALSE, FALSE, 12 );
   
   label = gtk_label_new( _("Image file type: ") );
   gtk_box_pack_start( GTK_BOX( hbox ), label, FALSE, FALSE, 12 );
   combo = gtk_combo_box_new_text( );
   g_signal_connect( combo, "changed", (GCallback)combo_box_text_changed_cb, UCVIEW_GCONF_DIR "/still_image_file_type" );
   gtk_box_pack_start( GTK_BOX( hbox ), combo, FALSE, FALSE, 12 );
   dlg->image_file_types_combo = combo;
   
   
/*    frame = gtk_frame_new( "Quality and Compression" ); */
/*    gtk_box_pack_start( GTK_BOX( vbox ), frame, TRUE, TRUE, 12 ); */

   return vbox;
}

static GtkWidget *create_time_lapse_page( SettingsDialog *dlg )
{
   GtkWidget *vbox;
   GtkWidget *vbox2;
   GtkWidget *hbox;
   GtkWidget *label;
   GtkWidget *combo;
   GtkWidget *check_button;
   GtkWidget *radio_button;
   GtkWidget *spin_button;
   GtkWidget *time_selection;
   GtkListStore *store;
   GtkTreeIter iter;
   GtkCellRenderer *renderer;
   gint sel;

   vbox = gtk_vbox_new( FALSE, 0 );
   
   hbox = gtk_hbox_new( FALSE, 0 );
   gtk_box_pack_start( GTK_BOX( vbox ), hbox, FALSE, FALSE, 12 );
   
   label = gtk_label_new( _("Time Lapse Action: ") );
   gtk_box_pack_start( GTK_BOX( hbox ), label, FALSE, FALSE, 12 );

   store = gtk_list_store_new( 2, G_TYPE_STRING, G_TYPE_INT );
   gtk_list_store_append( store, &iter );
   gtk_list_store_set( store, &iter, 0, _("Record Video"), 1, UCVIEW_TIME_LAPSE_VIDEO, -1 );
   gtk_list_store_append( store, &iter );
   gtk_list_store_set( store, &iter, 0, _("Image Sequence"), 1, UCVIEW_TIME_LAPSE_IMAGE_SEQUENCE, -1 );
   combo = gtk_combo_box_new_with_model( GTK_TREE_MODEL( store ) );
   gtk_box_pack_start( GTK_BOX( hbox ), combo, FALSE, FALSE, 12 );
   renderer = gtk_cell_renderer_text_new();
   gtk_cell_layout_pack_start( GTK_CELL_LAYOUT( combo ), renderer, FALSE );
   gtk_cell_layout_set_attributes( GTK_CELL_LAYOUT( combo ), renderer, "text", 0, NULL );
   sel = gconf_client_get_int( dlg->client, UCVIEW_GCONF_DIR "/time_lapse_action", NULL );
   g_object_set_data( G_OBJECT( combo ), "gconf_path", UCVIEW_GCONF_DIR "/time_lapse_action" );
   gtk_combo_box_set_active( GTK_COMBO_BOX( combo ), sel );
   g_signal_connect( combo, "changed", (GCallback)combo_box_model_changed_cb, (gpointer)1 );
   
   vbox2 = gtk_vbox_new( FALSE, 0 );
   gtk_box_pack_start( GTK_BOX( vbox ), vbox2, FALSE, FALSE, 12 );
   hbox = gtk_hbox_new( FALSE, 0 );
   gtk_box_pack_start( GTK_BOX( vbox2 ), hbox, FALSE, FALSE, 6 );
   gtk_box_pack_start( GTK_BOX( hbox ), gtk_label_new( _("Time between capture events:") ), FALSE, FALSE, 6 );
   hbox = gtk_hbox_new( FALSE, 0 );
   gtk_box_pack_start( GTK_BOX( vbox2 ), hbox, FALSE, FALSE, 12 );
   time_selection = ucview_time_selection_new( gconf_client_get_int( dlg->client, UCVIEW_GCONF_DIR "/time_lapse_delay", NULL ) );
   gtk_box_pack_start( GTK_BOX( hbox ), time_selection, FALSE, FALSE, 12 );   
   g_signal_connect( time_selection, "changed", (GCallback)time_changed_cb, "time_lapse_delay" );


   hbox = gtk_hbox_new( FALSE, 0 );
   gtk_box_pack_start( GTK_BOX( vbox ), hbox, FALSE, FALSE, 4 );
   
   sel = gconf_client_get_int( dlg->client, UCVIEW_GCONF_DIR "/time_lapse_stop_condition", NULL );
   
   check_button = gtk_check_button_new_with_label( _( "Automatically stop recording" ) );
   gtk_box_pack_start( GTK_BOX( hbox ), check_button, FALSE, FALSE, 12 );
   
   hbox = gtk_hbox_new( FALSE, 0 );
   gtk_box_pack_start( GTK_BOX( vbox ), hbox, FALSE, FALSE, 4 );
   vbox2 = gtk_vbox_new( FALSE, 0 );
   gtk_box_pack_start( GTK_BOX( hbox ), vbox2, FALSE, FALSE, 12 );
   g_object_set_data( G_OBJECT( check_button ), "vbox", vbox2 );
   
   hbox = gtk_hbox_new( FALSE, 0 );
   gtk_box_pack_start( GTK_BOX( vbox2 ), hbox, FALSE, FALSE, 4 );
   
   radio_button = gtk_radio_button_new_with_label( NULL, _( "After number of frames:" ) );
   gtk_box_pack_start( GTK_BOX( hbox ), radio_button, FALSE, FALSE, 12 );
   spin_button = gtk_spin_button_new_with_range( 1.0, 99999999, 1.0 );
   gtk_box_pack_start( GTK_BOX( hbox ), spin_button, FALSE, FALSE, 12 );
   gtk_spin_button_set_value( GTK_SPIN_BUTTON( spin_button ), 
			      (gdouble)gconf_client_get_int( dlg->client, UCVIEW_GCONF_DIR "/time_lapse_frames", NULL ) );
   g_signal_connect( radio_button, "toggled", (GCallback)stop_condition_toggled_cb, (gpointer)UCVIEW_STOP_CONDITION_FRAMES );
   g_signal_connect( spin_button, "value-changed", (GCallback)time_lapse_frames_changed_cb, dlg );
   
   hbox = gtk_hbox_new( FALSE, 0 );
   gtk_box_pack_start( GTK_BOX( vbox2 ), hbox, FALSE, FALSE, 4 );
   radio_button = gtk_radio_button_new_with_label_from_widget( GTK_RADIO_BUTTON( radio_button ), _( "After recording time:" ) );
   gtk_box_pack_start( GTK_BOX( hbox ), radio_button, FALSE, FALSE, 12 );
   hbox = gtk_hbox_new( FALSE, 0 );
   gtk_box_pack_start( GTK_BOX( vbox2 ), hbox, FALSE, FALSE, 12 );
   time_selection = ucview_time_selection_new( gconf_client_get_int( dlg->client, UCVIEW_GCONF_DIR "/time_lapse_time", NULL ) );
   gtk_box_pack_start( GTK_BOX( hbox ), time_selection, FALSE, FALSE, 12 );
   g_signal_connect( time_selection, "changed", (GCallback)time_changed_cb, "time_lapse_time" );
   g_signal_connect( radio_button, "toggled", (GCallback)stop_condition_toggled_cb, (gpointer)UCVIEW_STOP_CONDITION_TIME );
   g_signal_connect( check_button, "toggled", (GCallback)auto_stop_recording_toggled_cb, dlg );
   if( sel == UCVIEW_STOP_CONDITION_TIME )
   {
      gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( radio_button ), TRUE );
   }

   gtk_widget_set_sensitive( vbox2, gconf_client_get_bool( dlg->client, UCVIEW_GCONF_DIR "/time_lapse_auto_stop", NULL ) );

   gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( check_button ), 
				 gconf_client_get_bool( dlg->client, UCVIEW_GCONF_DIR "/time_lapse_auto_stop", NULL ) );
   
   gtk_widget_show_all( vbox );

   return vbox;
}



static void settings_dialog_init( SettingsDialog *dlg )
{
   GtkWidget *notebook;
   GtkWidget *page;
   GtkWidget *vbox;
   GtkWidget *hbox;
   GtkWidget *button;
   
   dlg->priv = SETTINGS_DIALOG_GET_PRIVATE( dlg );

   dlg->client = gconf_client_get_default();
   gconf_client_add_dir( dlg->client, UCVIEW_GCONF_DIR, GCONF_CLIENT_PRELOAD_NONE, NULL );   

   gtk_window_set_title( GTK_WINDOW( dlg ), _("ucview Preferences") );
   gtk_window_set_type_hint( GTK_WINDOW( dlg ), GDK_WINDOW_TYPE_HINT_DIALOG );
   gtk_container_set_border_width( GTK_CONTAINER( dlg ), 12 );
   
   vbox = gtk_vbox_new( FALSE, 12 );
   gtk_container_add( GTK_CONTAINER( dlg ), vbox );
   
   hbox = gtk_hbox_new( FALSE, 0 );
   gtk_box_pack_start( GTK_BOX( vbox ), hbox, TRUE, TRUE, 0 );

   notebook = gtk_notebook_new( );
   gtk_box_pack_start( GTK_BOX( hbox ), notebook, TRUE, TRUE, 0 );

   page = create_video_page(dlg);
   gtk_notebook_append_page( GTK_NOTEBOOK( notebook ), page, gtk_label_new( _("Video") ) );
   page = create_audio_page(dlg);
   gtk_notebook_append_page( GTK_NOTEBOOK( notebook ), page, gtk_label_new( _("Audio") ) );
   page = create_image_page(dlg);
   gtk_notebook_append_page( GTK_NOTEBOOK( notebook ), page, gtk_label_new( _("Image") ) );
   page = create_time_lapse_page( dlg );
   gtk_notebook_append_page( GTK_NOTEBOOK( notebook ), page, gtk_label_new( _("Time Lapse") ) );
   page = create_plugin_page( dlg );
   gtk_notebook_append_page( GTK_NOTEBOOK( notebook ), page, gtk_label_new( _("Plugins") ) );

   hbox = gtk_hbutton_box_new( );
   gtk_button_box_set_layout( GTK_BUTTON_BOX( hbox ), GTK_BUTTONBOX_END );
   gtk_box_pack_start( GTK_BOX( vbox ), hbox, FALSE, TRUE, 0 );
   
   button = gtk_button_new_from_stock( GTK_STOCK_CLOSE );
   g_signal_connect_swapped( button, "clicked", (GCallback)gtk_widget_hide, dlg );
   gtk_box_pack_end( GTK_BOX( hbox ), button, TRUE, FALSE, 12 );   
   
   gtk_widget_show_all( vbox );
   gtk_notebook_set_current_page( GTK_NOTEBOOK( notebook ), 0 );

}

static void settings_dialog_set_property( GObject *object, 
					  guint property_id, 
					  const GValue *value, 
					  GParamSpec *pspec )
{
   SettingsDialog *dlg = SETTINGS_DIALOG( object );

   switch( property_id )
   {
      case PROP_VIDEO_FILENAME:
	 gtk_file_chooser_set_filename( GTK_FILE_CHOOSER( dlg->video_file_button ), g_value_get_string( value ) );
	 break;

      case PROP_STILL_IMAGE_FILENAME:
	 gtk_file_chooser_set_filename( GTK_FILE_CHOOSER( dlg->still_image_file_button ), g_value_get_string( value ) );
	 break;	 

      case PROP_FORMAT:
	 unicap_copy_format( &dlg->priv->format, g_value_get_pointer( value ) );
	 break;

      default:
	 G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, pspec );
	 break;
   }
}

static void settings_dialog_get_property( GObject *object, 
					  guint property_id, 
					  GValue *value, 
					  GParamSpec *pspec )
{
   SettingsDialog *dlg = SETTINGS_DIALOG( object );

   switch( property_id )
   {
      case PROP_VIDEO_FILENAME:
	 g_value_set_string( value, gtk_file_chooser_get_filename( GTK_FILE_CHOOSER( dlg->video_file_button ) ) );
         break;

      case PROP_STILL_IMAGE_FILENAME:
	 g_value_set_string( value, gtk_file_chooser_get_filename( GTK_FILE_CHOOSER( dlg->still_image_file_button ) ) );
	 break;

      default:
	 G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, pspec );
	 break;
   }
}

void settings_dialog_update_plugins( SettingsDialog *dlg, UCViewWindow *window )
{
   GtkTreeModel *model;
   
   model = create_plugin_model( window );
   gtk_tree_view_set_model( GTK_TREE_VIEW( dlg->plugin_tv ), model );
   g_object_unref( model );
}

static gint get_combo_text_index( GtkWidget *combo, gchar *text )
{
   GtkTreeModel *model;
   GtkTreeIter iter;
   gboolean valid;
   gint index = -1;
   
   model = gtk_combo_box_get_model( GTK_COMBO_BOX( combo ) );
   valid = gtk_tree_model_get_iter_first( model, &iter );
   while( valid ){
      gchar *str_data;
      index++;
      gtk_tree_model_get( model, &iter, 0, &str_data, -1 );
      if( !strcmp( text, str_data ) )
	 return index;
      valid = gtk_tree_model_iter_next( model, &iter );
   }
   
   return -1;
}


static void image_file_type_added_cb( UCViewWindow *window, gchar *filetype, SettingsDialog *dlg )
{
   int index;
   gchar *str;
   gtk_combo_box_append_text( GTK_COMBO_BOX( dlg->image_file_types_combo ), filetype );
   
   str = gconf_client_get_string( dlg->client, UCVIEW_GCONF_DIR "/still_image_file_type", NULL );
   if( !str )
   {
      str = g_strdup( "jpeg" );
   }

   index = get_combo_text_index( dlg->image_file_types_combo, str );
   if( index >=0 )
      gtk_combo_box_set_active( GTK_COMBO_BOX( dlg->image_file_types_combo ), index );

   g_free( str );
}

static void image_file_type_removed_cb( UCViewWindow *window, gchar *filetype, SettingsDialog *dlg )
{
   int index;
   
   index = get_combo_text_index( dlg->image_file_types_combo, filetype );

   if( index >= 0 )
      gtk_combo_box_remove_text( GTK_COMBO_BOX( dlg->image_file_types_combo ), index );
}


GtkWidget *settings_dialog_new( UCViewWindow *window )
{
   SettingsDialog *dlg;
   
   dlg = g_object_new( SETTINGS_DIALOG_TYPE, NULL );
   dlg->ucv_window = window;

   g_signal_connect( window, "image_file_type_added", (GCallback)image_file_type_added_cb, dlg );
   g_signal_connect( window, "image_file_type_removed", (GCallback)image_file_type_removed_cb, dlg );
   

   return GTK_WIDGET( dlg );
}

