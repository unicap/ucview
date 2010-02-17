/*
** debayer_plugin.c
** 
** UCView RAW/Bayer Interpolation Plugin
**
*/

#include <ucview/ucview_plugin.h>
#include <ucview/ucview-window.h>
#include <unicap.h>
#include <unicapgtk.h>
#include <glib/gi18n.h>
#include <glib.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>

#include "debayer.h"
#include "plugin_ui.h"

#define PLUGIN_GCONF_DIR  UCVIEW_GCONF_DIR "/" UCVIEW_PLUGIN_GCONF_DIR "/debayer_plugin"

static void enable_cb( GtkAction *action, UCViewPlugin *plugin );
static void capture_new_frame_event_cb( unicap_event_t event, unicap_handle_t handle, unicap_data_buffer_t *buffer, void *data );
static void display_image_cb( UCViewWindow *ucv, unicap_data_buffer_t *buffer, UCViewPlugin *plugin );

static gint conf_get_int( GConfClient * client, gchar *key, gint dflt, gint min, gint max );


enum
{
   INTERP_NEAREST = 0,
/*    INTERP_BILINEAR, */
   INTERP_EDGE_SENSING,
};

   


struct private_data
{
      UCViewWindow *ucv;
      GConfClient *client;
      
      guint ui_merge_id;
      GtkActionGroup *actions;
      GtkBuilder *builder;
      
      gboolean active;

      gboolean auto_rbgain;
      gint interp_type;

      debayer_data_t debayer_data;


};

gchar *authors[] = 
{
   "Arne Caspari", 
   NULL
};

UCViewPlugin default_plugin_data = 
{
   "Debayer",                                // Name
   "RAW/Bayer Pattern Interpolation Plugin", // Description
   authors,                                  // Authors
   "Copyright 2007 Arne Caspari",            // Copyright
   "GPL",                                    // License
   "1.0",                                    // Version
   "http://www.unicap-imaging.org",          // Website

   NULL,                                     // user_ptr

   NULL,     
   NULL,                                     // record_frame_cb
   NULL,                                     // display_frame_cb
};


static GtkToggleActionEntry toggle_action_entries[] = 
{
   { "EnableDebayer", NULL, N_("Enable RAW/Bayer Interpolation"), NULL, N_("Enable RAW/Bayer Interpolation"), 
     G_CALLBACK( enable_cb ) }, 
};
static const guint n_toggle_action_entries = G_N_ELEMENTS( toggle_action_entries );



/*
  ==================================================================================
                        Helper Functions
  ==================================================================================
*/

static gint conf_get_int( GConfClient * client, gchar *key, gint dflt, gint min, gint max )
{
   GError *err = NULL;
   gint value;
   
   value = gconf_client_get_int( client, key, &err );
   if( err )
   {
      value = dflt;
   }

   if( value < min )
      value = min;

   if( value > max )
      value = max;
   
   return value;
}  


/*
  ==================================================================================
                        UCView Plugin UI Implementation
  ==================================================================================
*/

static void on_activate_ccm_toggled( GtkToggleButton *togglebutton, UCViewPlugin *plugin )
{
   struct private_data *data = plugin->user_ptr;

   data->debayer_data.use_ccm = gtk_toggle_button_get_active( togglebutton );
}

static void on_activate_rbgain_toggled( GtkToggleButton *togglebutton, UCViewPlugin *plugin )
{
   struct private_data *data = plugin->user_ptr;

   data->debayer_data.use_rbgain = gtk_toggle_button_get_active( togglebutton );
   
   gtk_widget_set_sensitive( GTK_WIDGET( gtk_builder_get_object( data->builder, "activate_auto_rbgain_check" ) ), 
			     gtk_toggle_button_get_active( togglebutton ) );
   gtk_widget_set_sensitive( GTK_WIDGET( gtk_builder_get_object( data->builder, "rscale" ) ), gtk_toggle_button_get_active( togglebutton ) );
   gtk_widget_set_sensitive( GTK_WIDGET( gtk_builder_get_object( data->builder, "bscale" ) ), gtk_toggle_button_get_active( togglebutton ) );
   

}

static void on_activate_auto_rbgain_toggled( GtkToggleButton *togglebutton, UCViewPlugin *plugin )
{
   struct private_data *data = plugin->user_ptr;

   data->auto_rbgain = gtk_toggle_button_get_active( togglebutton );
}

static void on_rgain_changed( GtkRange *range, UCViewPlugin *plugin )
{
   struct private_data *data = plugin->user_ptr;

   data->debayer_data.rgain = gtk_range_get_value( range ) * 4096;
}

static void on_bgain_changed( GtkRange *range, UCViewPlugin *plugin )
{
   struct private_data *data = plugin->user_ptr;

   data->debayer_data.bgain = gtk_range_get_value( range ) * 4096;
}
   
static void on_interp_type_changed( GtkComboBox *combo, UCViewPlugin *plugin )
{
   struct private_data *data = plugin->user_ptr;
   GtkTreeModel *model= gtk_combo_box_get_model( combo );
   GtkTreeIter iter;
   gint interp;
   
/*    gtk_combo_box_get_active_iter( combo, &iter ); */
/*    gtk_tree_model_get( model, &iter, 1, &interp, -1 ); */

   interp  = gtk_combo_box_get_active( combo );
   data->interp_type = interp;
}


#define CONNECT_HANDLER(name,handler)    \
   if( !strcmp( handler_name, name ) ) \
   {				       \
      g_signal_connect( object, signal_name, (GCallback)handler, user_data ); \
   }
   

static void connect_func( GtkBuilder *builder, GObject *object, const gchar *signal_name, const gchar *handler_name, 
			  GObject *connect_object, GConnectFlags flags, gpointer user_data )
{

   CONNECT_HANDLER("activate_ccm",on_activate_ccm_toggled);
   CONNECT_HANDLER("activate_rbgain", on_activate_rbgain_toggled);
   CONNECT_HANDLER("activate_auto_rbgain", on_activate_auto_rbgain_toggled);
   CONNECT_HANDLER("rgain_changed", on_rgain_changed);
   CONNECT_HANDLER("bgain_changed", on_bgain_changed);
   CONNECT_HANDLER("interp_type_combo_changed", on_interp_type_changed );
}



static GtkWidget *build_config_dlg( UCViewPlugin *plugin )
{      

   struct private_data *data = plugin->user_ptr;
   GtkWidget *window;
   GtkWidget *widget;
   GError *error = NULL;
   
   data->builder = gtk_builder_new();
   if( gtk_builder_add_from_string( data->builder, plugin_ui, -1, &error ) == 0 )
   {
      g_warning( "Failed to build the ui: %s", error->message );
      return NULL;
   }

   // gtk_builder_connect_signals does not work since this is a GModule
   gtk_builder_connect_signals_full( data->builder, connect_func, plugin );
   window = GTK_WIDGET( gtk_builder_get_object( data->builder, "window1" ) );
   g_object_ref( window );
   
   widget = GTK_WIDGET( gtk_builder_get_object( data->builder, "vbox1" ) );
   gtk_widget_show_all( widget );

   widget = GTK_WIDGET( gtk_builder_get_object( data->builder, "combobox1" ) );
   gtk_combo_box_set_active( GTK_COMBO_BOX( widget ), conf_get_int( data->client, PLUGIN_GCONF_DIR "/interp_type", 0, 0, 2 ) );


   return window;
}

/*
  ==================================================================================
                        UCView Plugin Interface Implementation
  ==================================================================================
*/

guint ucview_plugin_get_api_version()
{
   return 1<<16;
}

GtkWidget *ucview_plugin_configure( UCViewPlugin *plugin )
{
   return build_config_dlg( plugin );
}

static gboolean conversion_cb( unicap_data_buffer_t *dest, unicap_data_buffer_t *src, UCViewPlugin *plugin )
{
   struct private_data *data = plugin->user_ptr;
   gboolean converted = FALSE;
   struct timeval t1, t2;


   switch( src->format.fourcc )
   {
      case UCIL_FOURCC( 'G', 'R', 'E', 'Y' ):
      case UCIL_FOURCC( 'B', 'Y', '8', 0 ):
      case UCIL_FOURCC( 'Y', '8', '0', '0' ):
      {
	 switch( dest->format.fourcc )
	 {
	    case UCIL_FOURCC( 'R', 'G', 'B', '3' ):
	    {
	       if( data->auto_rbgain )
	       {
		  debayer_calculate_rbgain( src, &data->debayer_data.rgain, &data->debayer_data.bgain );
	       }
	       
	       gettimeofday( &t1, NULL );

	       switch( data->interp_type )
	       {
		  case INTERP_NEAREST:
		     debayer_ccm_rgb24_nn( dest, src, &data->debayer_data );
		     break;
		     
/* 		  case INTERP_BILINEAR: */
/* 		     break; */
		     
		  case INTERP_EDGE_SENSING:
		     debayer_ccm_rgb24_edge( dest, src, &data->debayer_data );
		     break;
		     
		  default:
		     break;
	       }

	       gettimeofday( &t2, NULL );
	       
	       t2.tv_sec -= t1.tv_sec;
	       t2.tv_usec += t2.tv_sec * 1000000;
	       t2.tv_usec -= t1.tv_usec;
	       
/* 	       g_message( "Execution time: %d usec\n", t2.tv_usec ); */
	       
	       converted = TRUE;
	    }
	    break;

	    case UCIL_FOURCC( 'U', 'Y', 'V', 'Y' ):
	    {
/* 	       debayer_ccm_uyvy( dest, src, &data->debayer_data ); */
/* 	       converted = TRUE; */
	    }
	    break;
	       

	   default:
	       break;
	 }
      }
      break;

      
      default:
	 break;
   }
   
   return converted;
}

/**
 * ucview_plugin_init: Called when the plugin gets loaded
 *
 */
void ucview_plugin_init( UCViewWindow *ucv, UCViewPlugin *plugin )
{
   struct private_data *data = g_malloc0( sizeof( struct private_data ) );
   
   g_memmove( plugin, &default_plugin_data, sizeof( UCViewPlugin ) );
   plugin->user_ptr = data;   

   data->actions = gtk_action_group_new( "Debayer Plugin Actions" );
   gtk_action_group_add_toggle_actions( data->actions, toggle_action_entries, n_toggle_action_entries, plugin );
   
   data->ucv = ucv;
   data->active = FALSE;
   data->client = gconf_client_get_default();

   
}


/**
 * ucview_plugin_unload: Called when the plugin gets unloaded
 *
 */
void ucview_plugin_unload( UCViewPlugin *plugin )
{
   g_free( plugin->user_ptr );
}


static gboolean enable_idle_func( UCViewPlugin *plugin )
{
   struct private_data *data = plugin->user_ptr;
   GtkAction *activate_action;
   
   activate_action = gtk_action_group_get_action( data->actions, "EnableDebayer" );
   gtk_toggle_action_set_active( GTK_TOGGLE_ACTION( activate_action ), 
				 gconf_client_get_bool( data->client, PLUGIN_GCONF_DIR "/active", NULL ) );
   return( FALSE );
}


/**
 * ucview_plugin_enable: Called when the plugin gets enabled via the Settings/PlugIns dialog
 *
 */
void ucview_plugin_enable( UCViewPlugin *plugin )
{
   struct private_data *data = plugin->user_ptr;
   GtkUIManager *ui;   
   int default_ccm[3][3] = 
      { {  1.3 * 1024, -0.2 * 1024, -0.1 * 1024 },
	{ -0.2 * 1024,  1.4 * 1024, -0.2 * 1024 },
	{ -0.1 * 1024, -0.2 * 1024,  1.3 * 1024 } 
      }; 

   ui = ucview_window_get_ui_manager( UCVIEW_WINDOW( data->ucv ) );   
   data->ui_merge_id = gtk_ui_manager_new_merge_id( ui );
   gtk_ui_manager_add_ui( ui, data->ui_merge_id, 
			  "ui/MenuBar/ImageMenu", "Enable Debayer", "EnableDebayer", 
			  GTK_UI_MANAGER_MENUITEM, FALSE );
   gtk_ui_manager_insert_action_group( ui, data->actions, 0 );

   memcpy( &data->debayer_data.ccm, &default_ccm, sizeof( default_ccm ) );
   data->debayer_data.use_ccm = 0;
   data->debayer_data.use_rbgain = 0;
   data->debayer_data.rgain = 1.0;
   data->debayer_data.bgain = 1.0;

   // 
   g_idle_add( enable_idle_func, plugin );
}

/**
 * ucview_plugin_disable: Called when the plugin gets disabled via the Settings/PlugIns dialog
 *
 */
void ucview_plugin_disable( UCViewPlugin *plugin )
{
   struct private_data *data = plugin->user_ptr;
   GtkUIManager *ui;   

   g_message( "Debayer plugin disable\n" );

   ui = ucview_window_get_ui_manager( UCVIEW_WINDOW( data->ucv ) );   
   gtk_ui_manager_remove_ui( ui, data->ui_merge_id );
   gtk_ui_manager_remove_action_group( ui, data->actions );

}

/*
  ==================================================================================
                               Plugin Implementation
  ==================================================================================
*/


static void enable_cb( GtkAction *action, UCViewPlugin *plugin )
{
   struct private_data *data = plugin->user_ptr;

   data->active = gtk_toggle_action_get_active( GTK_TOGGLE_ACTION( action ) );
   
   gconf_client_set_bool( data->client, PLUGIN_GCONF_DIR "/active",  data->active, NULL );
   if( data->active )
   {
      unsigned int fourcc;
      
      g_object_get( data->ucv->display, "backend_fourcc", &fourcc, NULL );
      if( fourcc != UCIL_FOURCC( 'R', 'G', 'B', '3' ) )
      {
	 unicap_format_t format;
	 unicapgtk_video_display_stop( UNICAPGTK_VIDEO_DISPLAY( data->ucv->display ) );
	 unicapgtk_video_display_get_format( UNICAPGTK_VIDEO_DISPLAY( data->ucv->display ), &format );
	 g_object_set( data->ucv->display, "backend_fourcc", UCIL_FOURCC( 'R', 'G', 'B', '3' ), "backend", "gtk", NULL );
	 unicapgtk_video_display_set_color_conversion_callback( UNICAPGTK_VIDEO_DISPLAY( data->ucv->display ), 
								conversion_cb, 
								plugin );
	 unicapgtk_video_display_set_format( UNICAPGTK_VIDEO_DISPLAY( data->ucv->display ), &format );
	 unicapgtk_video_display_start( UNICAPGTK_VIDEO_DISPLAY( data->ucv->display ) );
      }
   }
   else
   {
      unicapgtk_video_display_set_color_conversion_callback( UNICAPGTK_VIDEO_DISPLAY( data->ucv->display ), 
							     NULL, NULL );
   }      
}

