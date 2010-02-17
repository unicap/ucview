#include <gtk/gtk.h>
#include "ucvidplay.h"
#include <string.h>


static void uc_video_player_class_init( UCVideoPlayerClass *klass );
static void uc_video_player_init( UCVideoPlayer *player );
static void uc_video_player_destroy( GtkObject *obj );


static void on_rewind_clicked( GtkButton *button, UCVideoPlayer *player );
static void on_stop_clicked( GtkButton *button, UCVideoPlayer *player );
static void on_play_clicked( GtkButton *button, UCVideoPlayer *player );


G_DEFINE_TYPE( UCVideoPlayer, uc_video_player, GTK_TYPE_WINDOW );


static void uc_video_player_class_init( UCVideoPlayerClass *klass )
{
/*    GObjectClass *object_class = G_OBJECT_CLASS( klass ); */
   GtkObjectClass *gtk_object_class = GTK_OBJECT_CLASS( klass );

   gtk_object_class->destroy = uc_video_player_destroy;
};


static void uc_video_player_init( UCVideoPlayer *player )
{
   GtkWidget *da;
   GtkWidget *vbox;
   GtkWidget *hbox;
   GtkWidget *bbox;
   
   vbox = gtk_vbox_new( FALSE, 0 );
   gtk_container_add( GTK_CONTAINER( player ), vbox );
   hbox = gtk_hbox_new( FALSE, 0 );
   gtk_box_pack_start( GTK_BOX( vbox ), hbox, FALSE, TRUE, 0 );
   bbox = gtk_hbutton_box_new( );
   gtk_box_pack_start( GTK_BOX( hbox ), bbox, FALSE, FALSE, 0 );   
   player->button_rewind = gtk_button_new_from_stock( GTK_STOCK_MEDIA_REWIND );
   g_signal_connect( player->button_rewind, "clicked", ( GCallback )on_rewind_clicked, player );
   gtk_container_add( GTK_CONTAINER( bbox ), player->button_rewind );
   player->button_play = gtk_button_new_from_stock( GTK_STOCK_MEDIA_PLAY );
   g_signal_connect( player->button_play, "clicked", ( GCallback )on_play_clicked, player );
   gtk_container_add( GTK_CONTAINER( bbox ), player->button_play );
   player->button_stop = gtk_button_new_from_stock( GTK_STOCK_MEDIA_STOP );
   gtk_container_add( GTK_CONTAINER( bbox ), player->button_stop );   
   g_signal_connect( player->button_stop, "clicked", ( GCallback )on_stop_clicked, player );

   player->time_label = gtk_label_new( "" );
   gtk_box_pack_start( GTK_BOX( hbox ), player->time_label, TRUE, TRUE, 0 );

   player->drawing_area = gtk_drawing_area_new( );
   gtk_box_pack_start( GTK_BOX( vbox ), player->drawing_area, FALSE, FALSE, 0 );


   gtk_widget_set_size_request( player->drawing_area, 640, 480 );
   gtk_widget_show_all( vbox );
/*    gtk_widget_realize( da ); */
   gdk_flush();

   player->pipeline = NULL;
   player->update_pos_id = 0;

}

static void uc_video_player_destroy( GtkObject *obj )
{
   UCVideoPlayer *player = UC_VIDEO_PLAYER( obj );
   
   if( player->update_pos_id != 0 )
      g_source_remove( player->update_pos_id );

   if( player->pipeline ){
      gst_element_set_state( player->pipeline, GST_STATE_NULL );
      gst_element_get_state( player->pipeline, NULL, NULL, GST_SECOND * 10 );
      gst_object_unref( player->pipeline );
   }
   
}



static void on_play_clicked( GtkButton *button, UCVideoPlayer *player )
{
   gst_element_set_state( player->pipeline, GST_STATE_PLAYING );
}

static void on_stop_clicked( GtkButton *button, UCVideoPlayer *player )
{
   gst_element_set_state( player->pipeline, GST_STATE_PAUSED );
}

static void on_rewind_clicked( GtkButton *button, UCVideoPlayer *player )
{
   gst_element_seek_simple( player->pipeline, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT, 0 );
}

static GstBusSyncReply bus_reply( GstBus * bus, GstMessage * message, UCVideoPlayer *player )
{
   gint w, h;

   // ignore anything but 'prepare-xwindow-id' element messages
   if( GST_MESSAGE_TYPE( message ) != GST_MESSAGE_ELEMENT )
   {
      return GST_BUS_PASS;
   }
   

   if( !gst_structure_has_name( message->structure, "prepare-xwindow-id" ) )
   {
      return GST_BUS_PASS;
   }   

   gst_x_overlay_set_xwindow_id( GST_X_OVERLAY (GST_MESSAGE_SRC (message)),
				 GDK_WINDOW_XWINDOW( player->drawing_area->window ) );

   gst_message_unref (message);
   
   return GST_BUS_DROP;
}

static void on_new_decoded_pad( GstElement *element, GstPad *pad, gboolean arg2, GstBin *pipeline )
{
   GstPad *sinkpad = NULL;
   GstCaps *caps;
   GstStructure *str;

   caps = gst_pad_get_caps( pad );
   str = gst_caps_get_structure( caps, 0 );
   
   g_log( LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "on_new_decoded_pad: caps name=%s", gst_structure_get_name(str) );

   if( strstr( gst_structure_get_name( str ), "video" ) )
   {
      GstElement *queue;
      
      queue = gst_bin_get_by_name( GST_BIN( pipeline ), "colorspace" );
      if( !queue )
      {
	 g_warning( "No videoqueue!" );
	 return;
      }
      sinkpad = gst_element_get_pad( queue, "sink" );

      gst_object_unref( queue );
   }
   else if( strstr( gst_structure_get_name( str ), "audio" ) )
   {
      GstElement *queue;
      
      queue = gst_bin_get_by_name( GST_BIN( pipeline ), "audioqueue" );
      if( !queue )
      {
	 g_warning( "No audioqueue!" );
	 return;
      }
      sinkpad = gst_element_get_pad( queue, "sink" );

      gst_object_unref( queue );
   }
   
   gst_caps_unref( caps );
   
   if( sinkpad )
   {
      if( GST_PAD_LINK_FAILED( gst_pad_link( pad, sinkpad ) ) )
      {
	 g_error( "Pad link failed!\n" );
      }
   }
   
   gst_object_unref( sinkpad );
}

#define TIME_ARGS(t) \
        GST_CLOCK_TIME_IS_VALID (t) ? \
        (guint) (((GstClockTime)(t)) / (GST_SECOND * 60 * 60)) : 99, \
        GST_CLOCK_TIME_IS_VALID (t) ? \
        (guint) ((((GstClockTime)(t)) / (GST_SECOND * 60)) % 60) : 99, \
        GST_CLOCK_TIME_IS_VALID (t) ? \
        (guint) ((((GstClockTime)(t)) / GST_SECOND) % 60) : 99, \
        GST_CLOCK_TIME_IS_VALID (t) ? \
	   (guint) ((((GstClockTime)(t)) % GST_SECOND)/1000000) : 999999999


static gboolean update_position_cb( UCVideoPlayer *player )
{
   GstFormat fmt_time = GST_FORMAT_TIME;
   GstFormat fmt_frames = GST_FORMAT_BUFFERS;
   gint64 time_pos, time_len, frames_pos, frames_len;
   
   if( gst_element_query_position( player->pipeline, &fmt_time, &time_pos ) && 
       gst_element_query_duration( player->pipeline, &fmt_time, &time_len ) )
   {
      gchar str[512];
      
      g_snprintf( str, sizeof( str ), "%u:%02u:%02u.%03u / %u:%02u:%02u.%03u", TIME_ARGS( time_pos ), TIME_ARGS( time_len ) );
      
      
      gtk_label_set_text( GTK_LABEL( player->time_label ), str );
   }

   
   return TRUE;
}
   

gboolean uc_video_player_play_file( UCVideoPlayer *player, gchar *path )
{
   GstElement *source;
   GstElement *decodebin;
   GstElement *colorspace;
   GstElement *sink;
   GstBus *bus;

   if( player->pipeline ){
      gst_element_set_state( player->pipeline, GST_STATE_NULL );
      gst_element_get_state( player->pipeline, NULL, NULL, GST_SECOND * 10 );
      gst_object_unref( player->pipeline );
   }

   player->pipeline = gst_pipeline_new( NULL );
   source = gst_element_factory_make( "filesrc", "source" );
   g_object_set( source, "location", path, NULL );
   decodebin = gst_element_factory_make( "decodebin2", "decodebin" );
   g_signal_connect( decodebin, "new-decoded-pad", (GCallback)on_new_decoded_pad, player->pipeline );
   colorspace = gst_element_factory_make( "ffmpegcolorspace", "colorspace" );
   sink = gst_element_factory_make( "ximagesink", "sink" );
   
   bus = gst_pipeline_get_bus( GST_PIPELINE( player->pipeline ) );
   gst_bus_set_sync_handler( bus, (GstBusSyncHandler) bus_reply, player );   

   gst_bin_add_many( GST_BIN( player->pipeline ), source, decodebin, colorspace, sink, NULL );

   if( !gst_element_link( source, decodebin ) )
   {
      g_warning( "!link: source->decodebin" );
      return FALSE;
   }
   if( !gst_element_link( colorspace, sink ) )
   {
      g_warning( "!link: colorspace, sink" );
      return FALSE;
   }

   gst_element_set_state( player->pipeline, GST_STATE_PLAYING );

   if( player->update_pos_id !=0 )
      g_source_remove( player->update_pos_id );

   player->update_pos_id = g_timeout_add( 50, (GSourceFunc)update_position_cb, player );

   g_object_unref( bus );

   return TRUE;
}

GtkWidget *uc_video_player_new( )
{
   UCVideoPlayer *player;
   
   player = g_object_new( UC_TYPE_VIDEO_PLAYER, NULL );
   
   return GTK_WIDGET( player );
}
