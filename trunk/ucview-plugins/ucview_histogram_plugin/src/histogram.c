/*
** histogram.c
** 
** Made by (Arne Caspari)
** Login   <arne@localhost>
** 
*/


#include <ucview_plugin.h>
#include <ucview-window.h>
#include <unicap.h>
#include <unicapgtk.h>
#include <ucil.h>
#include <glib/gi18n.h>
#include <math.h>

#include <glib.h>
#include <string.h>
#include <stdlib.h>

#include <sys/time.h>
#include <time.h>

static gboolean enable_logging = ENABLE_LOGGING;

#define PLUGIN_GCONF_DIR  UCVIEW_GCONF_DIR "/" UCVIEW_PLUGIN_GCONF_DIR "/histogram"

#define DEFAULT_ALPHA 200

enum
{
   HISTOGRAM_EMBEDDED = 0, 
   HISTOGRAM_WINDOW = 1, 
   HISTOGRAM_SIDEBAR = 2, 
   HISTOGRAM_DISPLAY_MODE_LAST = HISTOGRAM_SIDEBAR
};



struct private_data
{
   UCViewWindow *ucv;
   GtkWidget *window;
   GtkWidget *da;
   GdkGC *gc;

   GConfClient *client;
   
   GtkWidget *opacity_scale;
   
   GMutex *mutex;
      
   guint ui_merge_id;
   GtkActionGroup *actions;

   gint notification_id;

   unicap_format_t format;
   unicap_data_buffer_t rgbbuffer;
   unicap_handle_t handle;
   
   gboolean active;
   gboolean enabled;
   
   gint timeout;
   
   guint values[3][256];
   gint alpha;
   gint display_mode;
   

   GList *cnxn_id_list;

   gulong video_format_changed_sigid;
   
      
};

enum
{
   POSITION_NW = 0, 
   POSITION_NE, 
   POSITION_SW, 
   POSITION_SE
};


static void capture_new_frame_event_cb( unicap_event_t event, unicap_handle_t handle, unicap_data_buffer_t *buffer, void *data );
static void display_image_cb( UCViewWindow *ucv, unicap_data_buffer_t *buffer, UCViewPlugin *plugin );
static void activate_plugin_cb( GtkAction *action, UCViewPlugin *plugin );
static void display_combo_changed_cb( GtkComboBox *combo, struct private_data *data );
static void scale_changed_int_cb( GtkRange *range, gchar *key );
static gboolean da_expose_cb( GtkWidget *da, GdkEvent *event, struct private_data *data );
static void draw_histogram( unicap_data_buffer_t *buffer, struct private_data *data, int position );
static void draw_window_histogram( struct private_data *data );
static void draw_embed_histogram( unicap_data_buffer_t *buffer, struct private_data *data, int position );
static gboolean check_visible( struct private_data *data );

static void conf_active_changed_cb( GConfClient *client, guint cnxn_id, GConfEntry *entry, struct private_data *data );
static void conf_alpha_value_changed_cb( GConfClient *client, guint cnxn_id, GConfEntry *entry, struct private_data *data );
static void conf_display_mode_changed_cb( GConfClient *client, guint cnxn_id, GConfEntry *entry, struct private_data *data );


void ucview_plugin_disable( UCViewPlugin *plugin );


gchar *authors[] = 
{
   "Arne Caspari", 
   NULL
};

UCViewPlugin default_plugin_data = 
{
   "Histogram", 
   "Histogram Display Plugin", 
   authors, 
   "Copyright 2008 Arne Caspari", 
   NULL,
   "1.0", 
   "http://www.unicap-imaging.org",
   NULL, 

   capture_new_frame_event_cb, 
   NULL,
   NULL,
};




static GtkToggleActionEntry toggle_action_entries[] = 
{
   { "DisplayHistogram", NULL, N_("Display Histogram"), NULL, N_( "Display Histogram" ), G_CALLBACK( activate_plugin_cb ) },
};
static const guint n_toggle_action_entries = G_N_ELEMENTS( toggle_action_entries );


static void log_func( const gchar *domain, GLogLevelFlags log_level, const gchar *message, gpointer data )
{
   if( enable_logging )
   {
      g_log_default_handler( domain, log_level, message, data );
   }
}

static void conf_active_changed_cb( GConfClient *client, guint cnxn_id, GConfEntry *entry, struct private_data *data )
{
   data->active = gconf_value_get_bool( gconf_entry_get_value( entry ) );
}

static void conf_alpha_value_changed_cb( GConfClient *client, guint cnxn_id, GConfEntry *entry, struct private_data *data )
{
   data->alpha = CLAMP( gconf_value_get_int( gconf_entry_get_value( entry ) ), 0, 255 );
   if( data->alpha == 0 )
   {
      data->alpha = DEFAULT_ALPHA;
   }
}

static void conf_display_mode_changed_cb( GConfClient *client, guint cnxn_id, GConfEntry *entry, struct private_data *data )
{
   data->display_mode = gconf_value_get_int( gconf_entry_get_value( entry ) );
   switch( data->display_mode )
   {
      case HISTOGRAM_WINDOW:
	 if( data->active )
	 {
	    gtk_widget_show_all( data->window );
	 }
	 break;
	 
      default:
      case HISTOGRAM_EMBEDDED:
	 gtk_widget_hide( data->window );
	 break;	 
   }
}

static void video_format_changed_cb( UCViewWindow *window, unicap_format_t *format, UCViewPlugin *plugin )
{
   struct private_data *data = plugin->user_ptr;
   
   if( data->rgbbuffer.data ){
      free( data->rgbbuffer.data );
   }

   unicap_copy_format( &data->format, format );

   unicap_void_format( &data->rgbbuffer.format );
   data->rgbbuffer.format.fourcc = UCIL_FOURCC( 'R', 'G', 'B', '3' );
   data->rgbbuffer.format.bpp = 24;
   data->rgbbuffer.format.size.width = format->size.width;
   data->rgbbuffer.format.size.height = format->size.height;
   data->rgbbuffer.buffer_size = data->rgbbuffer.format.buffer_size = format->size.width * format->size.height * 3;
   data->rgbbuffer.data = malloc( data->rgbbuffer.buffer_size );

}


static GtkWidget *build_config_dlg( UCViewPlugin *plugin )
{
   GtkWidget *window = gtk_window_new( GTK_WINDOW_TOPLEVEL );
   GtkWidget *vbox;
   GtkWidget *hbox;
   GtkWidget *combo;
   struct private_data *data = plugin->user_ptr;

   gtk_window_set_title( GTK_WINDOW( window ), _("Histogram Settings") );
   
   vbox = gtk_vbox_new( FALSE, 0 );
   gtk_container_add( GTK_CONTAINER( window ), vbox );
   hbox = gtk_hbox_new( FALSE, 0 );
   gtk_box_pack_start( GTK_BOX( vbox ), hbox, FALSE, FALSE, 6 );
   gtk_box_pack_start( GTK_BOX( hbox ), gtk_label_new( _( "Show histogram: " ) ), FALSE, FALSE, 6 );
   combo = gtk_combo_box_new_text();
   gtk_combo_box_append_text( GTK_COMBO_BOX( combo ), _( "Overlay video image" ) );
/*    gtk_combo_box_append_text( GTK_COMBO_BOX( combo ), _( "Sidebar" ) ); */
   gtk_combo_box_append_text( GTK_COMBO_BOX( combo ), _( "Window" ) );
   gtk_combo_box_set_active( GTK_COMBO_BOX( combo ), 
			     gconf_client_get_int( data->client, PLUGIN_GCONF_DIR "/display_mode", NULL ) );
   gtk_box_pack_start( GTK_BOX( hbox ), combo, FALSE, FALSE, 6 );
   g_signal_connect( combo, "changed", (GCallback)display_combo_changed_cb, data );
   
   hbox = gtk_hbox_new( FALSE, 0 );
   gtk_box_pack_start( GTK_BOX( vbox ), hbox, FALSE, FALSE, 6 );

   gtk_box_pack_start( GTK_BOX( hbox ), gtk_label_new( _( "Opacity:" ) ), FALSE, FALSE, 6 );
   data->opacity_scale = gtk_hscale_new_with_range( 1.0, 255.0, 1.0 );
   gtk_box_pack_start( GTK_BOX( hbox ), data->opacity_scale, TRUE, TRUE, 6 );
   gtk_range_set_value( GTK_RANGE( data->opacity_scale ), data->alpha );
   g_signal_connect( data->opacity_scale, "value-changed", (GCallback)scale_changed_int_cb, PLUGIN_GCONF_DIR "/alpha" );
   gtk_widget_set_size_request( data->opacity_scale, 100, -1 );
   if( gconf_client_get_int( data->client, PLUGIN_GCONF_DIR "/display_mode", NULL ) != HISTOGRAM_EMBEDDED )
   {
      gtk_widget_set_sensitive( data->opacity_scale, FALSE );
   }

   g_signal_connect( window, "delete-event", (GCallback)gtk_widget_destroy, NULL );
   gtk_widget_show_all( vbox );
   return window;
}

GtkWidget *ucview_plugin_configure( UCViewPlugin *plugin )
{
   return build_config_dlg( plugin );
}

guint ucview_plugin_get_api_version()
{
   return 1<<16;
}


void ucview_plugin_init( UCViewWindow *ucv, UCViewPlugin *plugin )
{
   struct private_data *data = g_malloc0( sizeof( struct private_data ) );

   
   g_log_set_handler( LOG_DOMAIN, G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION,
		      log_func, NULL );
   
   g_log( LOG_DOMAIN, G_LOG_LEVEL_INFO, "histogram plugin init" );

   g_memmove( plugin, &default_plugin_data, sizeof( UCViewPlugin ) );
   plugin->user_ptr = data;   

   data->actions = gtk_action_group_new( "Histogram Actions" );
   gtk_action_group_add_toggle_actions( data->actions, toggle_action_entries, n_toggle_action_entries, plugin );
   
   data->client = gconf_client_get_default();
   data->ucv = ucv;
   data->active = FALSE;
   data->mutex = g_mutex_new();
   data->rgbbuffer.data = NULL;
   data->enabled = FALSE;


   unicap_void_format( &data->format );
}

void ucview_plugin_unload( UCViewPlugin *plugin )
{
   struct private_data *data = plugin->user_ptr;

   g_log( LOG_DOMAIN, G_LOG_LEVEL_INFO, "histogram plugin unload" );

   if( data->enabled )
   {
      ucview_plugin_disable( plugin );
   }

   

   g_object_unref( data->client );

   g_mutex_free( data->mutex );   
   g_free( plugin->user_ptr );
}


void ucview_plugin_enable( UCViewPlugin *plugin )
{
   struct private_data *data = plugin->user_ptr;
   GtkAction *activate_action;
   GtkUIManager *ui;
   unicap_format_t format;

   g_log( LOG_DOMAIN, G_LOG_LEVEL_INFO, "histogram plugin enable" );

   data->cnxn_id_list = NULL;
   data->enabled = TRUE;

   data->video_format_changed_sigid = g_signal_connect( data->ucv, "video_format_changed", (GCallback)video_format_changed_cb, plugin );

   data->rgbbuffer.data = NULL;

   ui = ucview_window_get_ui_manager( UCVIEW_WINDOW( data->ucv ) );
   
   data->ui_merge_id = gtk_ui_manager_new_merge_id( ui );
   gtk_ui_manager_add_ui( ui, data->ui_merge_id, 
			  "ui/MenuBar/ImageMenu", "Display Histogram", "DisplayHistogram", 
			  GTK_UI_MANAGER_MENUITEM, FALSE );
   gtk_ui_manager_insert_action_group( ui, data->actions, 0 );
   g_signal_connect( data->ucv, "display_image", (GCallback) display_image_cb, plugin );


   data->window = gtk_window_new( GTK_WINDOW_TOPLEVEL );
   gtk_window_set_title( GTK_WINDOW( data->window ), _("Histogram") );
   g_signal_connect( G_OBJECT( data->window ), "delete-event", (GCallback)gtk_widget_hide_on_delete, NULL );
   data->da = gtk_drawing_area_new();
   gtk_widget_show( data->da );
   gtk_container_add( GTK_CONTAINER( data->window ), data->da );
   gtk_window_set_transient_for( GTK_WINDOW( data->window ), GTK_WINDOW( data->ucv ) );
   gtk_window_set_default_size( GTK_WINDOW( data->window ), 256, 256 );
   g_signal_connect( data->da, "expose-event", (GCallback)da_expose_cb, data );


   activate_action = gtk_action_group_get_action( data->actions, "DisplayHistogram" );
   gtk_toggle_action_set_active( GTK_TOGGLE_ACTION( activate_action ), 
				 gconf_client_get_bool( data->client, PLUGIN_GCONF_DIR "/active", NULL ) );
   
   data->cnxn_id_list = g_list_append( data->cnxn_id_list, 
				       GUINT_TO_POINTER( 
					  gconf_client_notify_add( data->client, PLUGIN_GCONF_DIR "/alpha",
								   (GConfClientNotifyFunc) conf_alpha_value_changed_cb, data, NULL, NULL ) ) );
   data->cnxn_id_list = g_list_append( data->cnxn_id_list, 
				       GUINT_TO_POINTER( 
					  gconf_client_notify_add( data->client, PLUGIN_GCONF_DIR "/active",
								   (GConfClientNotifyFunc) conf_active_changed_cb, data, NULL, NULL ) ) );
   data->cnxn_id_list = g_list_append( data->cnxn_id_list, 
				       GUINT_TO_POINTER( 
					  gconf_client_notify_add( data->client, PLUGIN_GCONF_DIR "/display_mode",
								   (GConfClientNotifyFunc) conf_display_mode_changed_cb, data, NULL, NULL ) ) );

   gconf_client_notify( data->client, PLUGIN_GCONF_DIR "/alpha" );
   gconf_client_notify( data->client, PLUGIN_GCONF_DIR "/active" );
   gconf_client_notify( data->client, PLUGIN_GCONF_DIR "/display_mode" );

   ucview_window_get_video_format( data->ucv, &format );
   video_format_changed_cb( data->ucv, &format, plugin );
}

void ucview_plugin_disable( UCViewPlugin *plugin )
{
   struct private_data *data = plugin->user_ptr;
   GList *entry;

   GtkUIManager *ui = ucview_window_get_ui_manager( UCVIEW_WINDOW( data->ucv ) );

   g_log( LOG_DOMAIN, G_LOG_LEVEL_INFO, "histogram plugin disable" );

   gtk_ui_manager_remove_ui( ui, data->ui_merge_id );
   gtk_ui_manager_remove_action_group( ui, data->actions );

   for( entry = g_list_first( data->cnxn_id_list ); entry; entry = g_list_next( entry ) )
   {
      gconf_client_notify_remove( data->client, GPOINTER_TO_UINT( entry->data ) );
   }   

   gtk_widget_destroy( data->window );

   g_signal_handler_disconnect( data->ucv, data->video_format_changed_sigid );

   if( data->rgbbuffer.data )
   {
      free( data->rgbbuffer.data );
   }
   data->rgbbuffer.data = NULL;
}

static void display_combo_changed_cb( GtkComboBox *combo, struct private_data *data )
{
   gconf_client_set_int( data->client, PLUGIN_GCONF_DIR "/display_mode", gtk_combo_box_get_active( combo ), NULL );
   gtk_widget_set_sensitive( data->opacity_scale, gtk_combo_box_get_active( combo ) == HISTOGRAM_EMBEDDED );
   
}



static void scale_changed_int_cb( GtkRange *range, gchar *key )
{
   GConfClient *client = gconf_client_get_default();
   gconf_client_set_int( client, key, (gint)gtk_range_get_value( range ), NULL );
   g_object_unref( client );
}



static gboolean da_expose_cb( GtkWidget *da, GdkEvent *event, struct private_data *data )
{
   if( !data->gc )
   {
      data->gc = gdk_gc_new( data->da->window );
   }

   g_mutex_lock( data->mutex );
   draw_window_histogram( data );
   g_mutex_unlock( data->mutex );

   return TRUE;
}


static void activate_plugin_cb( GtkAction *action, UCViewPlugin *plugin )
{
   struct private_data *data = plugin->user_ptr;
   gboolean active = gtk_toggle_action_get_active( GTK_TOGGLE_ACTION( action ) );

   gconf_client_set_bool( data->client, PLUGIN_GCONF_DIR "/active",  active, NULL );
   if( active )
   {
      if( data->display_mode == HISTOGRAM_WINDOW )
      {
	 gtk_window_present( GTK_WINDOW( data->window ) );
      }
   }
   else
   {
      gtk_widget_hide( data->window );
   }
}

static guint getmax( struct private_data *data )
{
   guint max = 0;
   int i,j;
   

   for( i = 0; i < 3; i++ )
   {
      for( j = 0; j < 256; j++ )
      {
	 if( data->values[i][j] > max )
	 {
	    max = data->values[i][j];
	 }
      }
   }
   
   return max;
}

static void draw_embed_histogram( unicap_data_buffer_t *buffer, struct private_data *data, int position )
{
   int i;
   double vscale;
   int x = 4, y = 260;
   
   vscale = 256.0 / (double)getmax( data );

   // Do not paint the histogram if the video format resolution is too small
   if( ( buffer->format.size.width < 260 ) ||
       ( buffer->format.size.height < 260 ) )
      return;
   
   switch( position )
   {
      case POSITION_NW:
	 break;
	 
      case POSITION_NE:
	 x = buffer->format.size.width - 260;
	 break;
	 
      case POSITION_SW:
	 y = buffer->format.size.height - 5;
	 break;
	 
      case POSITION_SE:
	 y = buffer->format.size.height - 5;
	 x = buffer->format.size.width - 260;
	 break;
   }
   
   for( i = 0; i < 256; i++ )
   {
      ucil_color_t color, pixel;
      int min;
      int ya;
      int yb = y;
      
      color.colorspace = UCIL_COLORSPACE_RGB32;
      color.rgb32.a = data->alpha;
      color.rgb32.r = color.rgb32.g = color.rgb32.b = 0xff;

      pixel.colorspace = ucil_get_colorspace_from_fourcc( buffer->format.fourcc );


      while( color.rgb32.r || color.rgb32.g || color.rgb32.b )
      {
	 int j;
	 ucil_convert_color( &pixel, &color );
	 min = MIN( data->values[0][i], MIN( data->values[1][i], data->values[2][i] ) );
	 ya = y - ( min * vscale );
	 for( j = ya; j<yb; j++ )
	 {
	    ucil_set_pixel_alpha( buffer, &pixel, data->alpha, x + i, j );
	 }
	 yb = ya;
	 if( data->values[0][i] == min )
	 {
	    color.rgb32.r = 0;
	    data->values[0][i] = G_MAXUINT32;
	 }
	 if( data->values[1][i] == min )
	 {
	    color.rgb32.g = 0;
	    data->values[1][i] = G_MAXUINT32;
	 }
	 if( data->values[2][i] == min )
	 {
	    color.rgb32.b = 0;
	    data->values[2][i] = G_MAXUINT32;
	 }
	 
      }
   }
}

static void draw_window_histogram( struct private_data *data )
{
   int i;
   double vscale;
   double hstep;
   int y = data->da->allocation.height;
   GdkColor color;
   guint values[3][256];
   
   vscale = data->da->allocation.height / (double)getmax( data );   
   hstep = data->da->allocation.width / 256.0;

   color.red = color.green = color.blue = 0;      

   gdk_gc_set_rgb_fg_color( data->gc, &color );
   gdk_draw_rectangle( data->da->window, data->gc, TRUE, 0, 0, data->da->allocation.width, data->da->allocation.height );

   memcpy( values, data->values, sizeof( values ) );
   
   for( i = 0; i < 256; i++ )
   {
      int min;
      int ya;
      int yb = y;

      color.red = color.green = color.blue = 0xffff;      
      
      while( color.red || color.green || color.blue )
      {
	 int x = 0;
	 min = MIN( values[0][i], MIN( values[1][i], values[2][i] ) );
	 ya = y - ( min * vscale );	 
	 gdk_gc_set_rgb_fg_color( data->gc, &color );

	 gdk_draw_rectangle( data->da->window, data->gc, TRUE, (x+i) * hstep, ya, ceil( hstep ), yb-ya  );

	 yb = ya;
	 if( values[0][i] == min )
	 {
	    color.red = 0;
	    values[0][i] = G_MAXUINT32;
	 }
	 if( values[1][i] == min )
	 {
	    color.green = 0;
	    values[1][i] = G_MAXUINT32;
	 }
	 if( values[2][i] == min )
	 {
	    color.blue = 0;
	    values[2][i] = G_MAXUINT32;
	 }
      }
   }
}

static gboolean check_visible( struct private_data *data )
{
   gboolean ret = TRUE;
   switch( data->display_mode )
   {
      case HISTOGRAM_WINDOW:
      {
	 ret = GTK_WIDGET_VISIBLE( data->window );
      }
      break;
      
      default:
	 break;
   }
   
   return ret;
}


static void draw_histogram( unicap_data_buffer_t *buffer, struct private_data *data, int position )
{   
   if( check_visible( data ) )
   {
      switch( data->display_mode )
      {
	 default:
	 case HISTOGRAM_EMBEDDED:
	 {
	    draw_embed_histogram( buffer, data, position );
	 }
	 break;
      
	 case HISTOGRAM_WINDOW:
	 {
	    GdkRectangle rect;
	    rect.x = 0;
	    rect.y = 0;
	    rect.width = data->da->allocation.width;
	    rect.height = data->da->allocation.height;
	    
	    gdk_window_invalidate_rect( data->da->window, &rect, FALSE );
	 }
	 break;
      }
   } 
}

static void display_image_cb( UCViewWindow *ucv, unicap_data_buffer_t *buffer, UCViewPlugin *plugin )
{
   struct private_data *data = plugin->user_ptr;

   if( data->active && check_visible( data ) && data->rgbbuffer.data )
   {
      unicap_data_buffer_t *rgbbuffer;
      int i;

      
      if( ( buffer->format.fourcc != UCIL_FOURCC( 'R', 'G', 'B', '3' ) ) ||
	  ( buffer->format.fourcc != UCIL_FOURCC( 'R', 'G', 'B', 0 ) ) )
      {
	 ucil_convert_buffer( &data->rgbbuffer, buffer );
	 rgbbuffer = &data->rgbbuffer;
      }
      else
      {
	 rgbbuffer = buffer;
      }
      

      g_mutex_lock( data->mutex );
      
      for( i = 0; i < 256; i++ )
      {
	 data->values[0][i] = data->values[1][i] = data->values[2][i] = 0;
      }

      for( i = 0; i < ( buffer->format.size.width * buffer->format.size.height ); i++ )
      {
	 data->values[0][rgbbuffer->data[ (i*3) + 0 ]]++;
	 data->values[1][rgbbuffer->data[ (i*3) + 1 ]]++;
	 data->values[2][rgbbuffer->data[ (i*3) + 2 ]]++;
      }

      g_mutex_unlock( data->mutex );
      
      draw_histogram( buffer, data, POSITION_SE );
   }
}




      
      

static void capture_new_frame_event_cb( unicap_event_t event, unicap_handle_t handle, unicap_data_buffer_t *buffer, void *_data )
{
/*    struct private_data *data = _data;    */
   
}

