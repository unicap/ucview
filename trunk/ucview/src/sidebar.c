/*
** sidebar.c
** 
** Made by (Arne Caspari)
** Login   <arne@localhost>
** 
** Started on  Wed Sep  5 07:54:33 2007 Arne Caspari
*/

#include "sidebar.h"
#include "ucview.h"


#include <gtk/gtk.h>
#include <ucil.h>
#include <unicapgtk.h>


static void sidebar_class_init( SidebarClass *klass );
static void sidebar_init( Sidebar *box );
static void sidebar_set_property( GObject *obj, guint property_id, const GValue *value, GParamSpec *pspec );
static void sidebar_get_property( GObject *obj, guint property_id, GValue *value, GParamSpec *pspec );

#define SIDEBAR_GET_PRIVATE( obj )( G_TYPE_INSTANCE_GET_PRIVATE( ( obj ), SIDEBAR_TYPE, SidebarPrivate ) )

struct _SidebarPrivate
{
      GtkWidget *scrolled_window;
      GtkWidget *images_box;
      UCViewWindow *ucv;

      gint image_width;
};

G_DEFINE_TYPE( Sidebar, sidebar, GTK_TYPE_HBOX );

static void sidebar_class_init( SidebarClass *klass )
{
   GObjectClass *object_class = G_OBJECT_CLASS( klass );
/*    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS( klass ); */
   
   object_class->set_property = sidebar_set_property;
   object_class->get_property = sidebar_get_property;
   
   g_type_class_add_private( object_class, sizeof( SidebarPrivate ) );
}

static void sidebar_init( Sidebar *box )
{
   GConfClient *client = gconf_client_get_default();
   gint image_width;

   box->priv = SIDEBAR_GET_PRIVATE( box );


   image_width = gconf_client_get_int( client, UCVIEW_GCONF_DIR "/sidebar_image_width", NULL );
   if( image_width == 0 )
   {
      image_width = 128;
   }

   box->priv->image_width = image_width;

   box->priv->scrolled_window = gtk_scrolled_window_new( NULL, NULL );
   box->priv->images_box = gtk_vbox_new( FALSE, 4 );
   gtk_scrolled_window_add_with_viewport( GTK_SCROLLED_WINDOW( box->priv->scrolled_window ), box->priv->images_box );
   gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW( box->priv->scrolled_window ), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC );
   
   gtk_widget_show_all( box->priv->scrolled_window );
   gtk_box_pack_start( GTK_BOX( box ), box->priv->scrolled_window, TRUE, TRUE, 0 );
   gtk_widget_show_all( GTK_WIDGET( box ) );
   
   return;
}

GtkWidget *sidebar_new( UCViewWindow *ucv )
{
   Sidebar *sidebar;
   sidebar = g_object_new( SIDEBAR_TYPE, NULL );

   sidebar->priv->ucv = ucv;
   
   return GTK_WIDGET( sidebar );
}


static void sidebar_set_property( GObject *obj, guint property_id, const GValue *value, GParamSpec *pspec )
{
/*    Sidebar *box = SIDEBAR( obj ); */
   
   switch( property_id )
   {
	 
      default: 
	 G_OBJECT_WARN_INVALID_PROPERTY_ID( obj, property_id, pspec );
	 break;
   }
}

static void sidebar_get_property( GObject *obj, guint property_id, GValue *value, GParamSpec *pspec )
{
/*    Sidebar *box = SIDEBAR( obj ); */
   
   switch( property_id )
   {
      
      default: 
	 G_OBJECT_WARN_INVALID_PROPERTY_ID( obj, property_id, pspec );
	 break;
   }
}


static gboolean enter_event_cb( GtkWidget *box, GdkEventCrossing *event, gpointer data )
{
   GdkColormap *colormap;
   GdkColor color;      

   color.red = 0xaa00;
   color.green = 0xaa00;
   color.blue = 0xff00;
   colormap = gtk_widget_get_colormap( box );
   gdk_rgb_find_color( colormap, &color );

   gdk_rgb_find_color( colormap, &color );
   gtk_widget_modify_bg( box, GTK_STATE_NORMAL, &color );

   return TRUE;
}

static gboolean leave_event_cb( GtkWidget *box, GdkEventCrossing *event, gpointer data )
{
   gtk_widget_modify_bg( box, GTK_STATE_NORMAL, NULL );

   return TRUE;
}

static gboolean button_release_cb( GtkWidget *box, GdkEventButton *event, Sidebar *sidebar )
{
   GtkAction *pause_action;
   gchar *path;
   GdkPixbuf *pixbuf;
   

   path = g_object_get_data( G_OBJECT( box ), "filename" );
   
   pixbuf = gdk_pixbuf_new_from_file( path, NULL );
   
   if( pixbuf )
   {
      unicap_data_buffer_t buffer;

      unicap_void_format( &buffer.format );
      
      buffer.format.size.width = gdk_pixbuf_get_width( pixbuf );
      buffer.format.size.height = gdk_pixbuf_get_height( pixbuf );
      buffer.format.fourcc = gdk_pixbuf_get_has_alpha( pixbuf ) ? UCIL_FOURCC( 'R', 'G', 'B', '4' ) : UCIL_FOURCC( 'R', 'G', 'B', '3' );
      buffer.format.bpp = gdk_pixbuf_get_has_alpha( pixbuf ) ? 32: 24;
      buffer.buffer_size = buffer.format.buffer_size = buffer.format.size.width * buffer.format.size.height * buffer.format.bpp / 8;
      buffer.data = gdk_pixbuf_get_pixels( pixbuf );

      pause_action = gtk_ui_manager_get_action( ucview_window_get_ui_manager( UCVIEW_WINDOW( sidebar->priv->ucv ) ), "/MenuBar/ViewMenu/Pause" );
      gtk_toggle_action_set_active( GTK_TOGGLE_ACTION( pause_action ), TRUE );

      unicapgtk_video_display_set_still_image( UNICAPGTK_VIDEO_DISPLAY( sidebar->priv->ucv->display ), &buffer );
      
      g_object_unref( pixbuf );
   }

   return TRUE;
}



      
gboolean sidebar_add_image( Sidebar *sidebar, unicap_data_buffer_t *image, gchar *path )
{
   GtkWidget *event_box;
   GdkPixbuf *pixbuf;
   GdkPixbuf *scaled_pixbuf;
   GtkWidget *box;
   GtkWidget *image_widget;
   GtkWidget *label;
   gchar *filename;
   gchar *text;
   guchar *pixels;
   unicap_data_buffer_t srcbuf;

   int height;
   double factor;

   unicap_copy_format( &srcbuf.format, &image->format );
   srcbuf.format.bpp = 24;
   srcbuf.format.fourcc = UCIL_FOURCC( 'R', 'G', 'B', '3' );
   srcbuf.format.buffer_size = srcbuf.buffer_size = image->format.size.width * image->format.size.height * 3;
   pixels = g_malloc( srcbuf.buffer_size );
   srcbuf.data = pixels;
   ucil_convert_buffer( &srcbuf, image );

   pixbuf = gdk_pixbuf_new_from_data( pixels, 
				      GDK_COLORSPACE_RGB, 
				      FALSE, 
				      8, 
				      image->format.size.width, 
				      image->format.size.height, 
				      image->format.size.width * 3, 
				      (GdkPixbufDestroyNotify)g_free, NULL );
   if( !pixbuf )
   {
      g_free( pixels );
      return FALSE;
   }

   factor = (double)sidebar->priv->image_width / (double)image->format.size.width;
   height = image->format.size.height * factor;

   scaled_pixbuf = gdk_pixbuf_scale_simple( pixbuf, sidebar->priv->image_width, height, GDK_INTERP_HYPER );

   g_object_unref( pixbuf );
   
   filename = g_path_get_basename( path );
   text = g_strdup_printf( "<i>%s</i>", filename );
   g_free( filename );

   event_box = gtk_event_box_new( );
   
   box = gtk_vbox_new( FALSE, 2 );
   gtk_container_add( GTK_CONTAINER( event_box ), box );
   
   image_widget = gtk_image_new_from_pixbuf( scaled_pixbuf );
   label = gtk_label_new( text );
   g_free( text );
   gtk_label_set_use_markup( GTK_LABEL( label ), TRUE );
   
   gtk_box_pack_start( GTK_BOX( box ), image_widget, FALSE, TRUE, 4 );
   gtk_box_pack_start( GTK_BOX( box ), label, FALSE, TRUE, 0 );

   gtk_event_box_set_above_child( GTK_EVENT_BOX( event_box ), TRUE );

   gtk_widget_show_all( event_box );

   g_object_set_data( G_OBJECT( event_box ), "filename", g_strdup( path ) );

   g_signal_connect( event_box, "enter-notify-event", (GCallback)enter_event_cb, NULL );
   g_signal_connect( event_box, "leave-notify-event", (GCallback)leave_event_cb, NULL );
   g_signal_connect( event_box, "button-release-event", (GCallback)button_release_cb, sidebar );
   
   
   gtk_box_pack_start( GTK_BOX( sidebar->priv->images_box ), event_box, FALSE, FALSE, 0 );
   gtk_box_reorder_child( GTK_BOX( sidebar->priv->images_box ), event_box, 0 );

   return TRUE;
}
   
   
