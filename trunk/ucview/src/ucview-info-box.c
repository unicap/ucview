/*
** ucview_info_box.c
** 
** Made by (Arne Caspari)
** Login   <arne@localhost>
** 
** Started on  Wed Jul 18 17:29:55 2007 Arne Caspari
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

/*
 * ucview_info_box - a box meant to be placed on top of an
 * unicap_video_display to display important messages
 *
 * some parts are heavily inspired by the gedit-message-area and
 * should closely resemble the behaviour of a gedit-message area
 */

#include "ucview-info-box.h"
#include <gtk/gtk.h>
#include <glib/gi18n.h>

static void ucview_info_box_class_init( UCViewInfoBoxClass *klass );
static void ucview_info_box_init( UCViewInfoBox *box );
static void ucview_info_box_set_property( GObject *obj, guint property_id, const GValue *value, GParamSpec *pspec );
static void ucview_info_box_get_property( GObject *obj, guint property_id, GValue *value, GParamSpec *pspec );
static gboolean paint_ucview_info_box( GtkWidget *widget, GdkEventExpose *event, gpointer user_data );
static void style_set (GtkWidget *widget, GtkStyle  *prev_style);
static void ucview_info_box_response( UCViewInfoBox *box, gint response_id );

#define UCVIEW_INFO_BOX_GET_PRIVATE( obj )( G_TYPE_INSTANCE_GET_PRIVATE( ( obj ), UCVIEW_INFO_BOX_TYPE, UCViewInfoBoxPrivate ) )

struct _UCViewInfoBoxPrivate
{
      GtkWidget *image_box;
      GtkWidget *content_box;
      GtkWidget *action_box;

      GtkWidget *image;
      GtkWidget *content;
      
      gboolean changing_style;
};

typedef struct _ResponseData ResponseData;

struct _ResponseData
{
      gint response_id;
};


G_DEFINE_TYPE( UCViewInfoBox, ucview_info_box, GTK_TYPE_HBOX );


enum
{
   PROP_0, 
   PROP_IMAGE, 
   PROP_TEXT, 
};

enum
{
   RESPONSE, 
   LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];

static void ucview_info_box_class_init( UCViewInfoBoxClass *klass )
{
   GObjectClass *object_class = G_OBJECT_CLASS( klass );
   GtkWidgetClass *widget_class = GTK_WIDGET_CLASS( klass );
   
   object_class->set_property = ucview_info_box_set_property;
   object_class->get_property = ucview_info_box_get_property;
   
   widget_class->style_set = style_set;

   g_type_class_add_private( object_class, sizeof( UCViewInfoBoxPrivate ) );

   g_object_class_install_property( object_class, 
				    PROP_IMAGE, 
				    g_param_spec_object( "image", 
							 "Image", 
							 "An image widget to show in the image area", 
							 GTK_TYPE_WIDGET, 
							 G_PARAM_READWRITE ) );
   g_object_class_install_property( object_class, 
				    PROP_TEXT, 
				    g_param_spec_object( "text", 
							 "Text", 
							 "A text widget to show in the text area", 
							 GTK_TYPE_WIDGET, 
							 G_PARAM_READWRITE ) );

   signals[RESPONSE] = g_signal_new( "response", 
				     G_OBJECT_CLASS_TYPE (klass),
				     G_SIGNAL_RUN_LAST,
				     G_STRUCT_OFFSET (UCViewInfoBoxClass, response),
				     NULL, NULL,
				     g_cclosure_marshal_VOID__INT,
				     G_TYPE_NONE, 1,
				     G_TYPE_INT );
   

}

static void ucview_info_box_init( UCViewInfoBox *box )
{
   box->priv = UCVIEW_INFO_BOX_GET_PRIVATE( box );
   box->priv->image_box = gtk_vbox_new( FALSE, 0 );
   gtk_box_pack_start( GTK_BOX( box ), box->priv->image_box, FALSE, FALSE, 12 );
   box->priv->content_box = gtk_vbox_new( FALSE, 0 );
   gtk_box_pack_start( GTK_BOX( box ), box->priv->content_box, TRUE, TRUE, 12 );
   box->priv->action_box = gtk_vbox_new( FALSE, 12 );
   gtk_box_pack_start( GTK_BOX( box ), box->priv->action_box, FALSE, TRUE, 12 );

   box->priv->image = NULL;
   box->priv->content = NULL;

   g_signal_connect( box, "expose_event", G_CALLBACK( paint_ucview_info_box ), NULL );
   return;
}

static void ucview_info_box_set_property( GObject *obj, guint property_id, const GValue *value, GParamSpec *pspec )
{
   UCViewInfoBox *box = UCVIEW_INFO_BOX( obj );
   
   switch( property_id )
   {
      case PROP_IMAGE:
	 box->priv->image = g_value_get_object( value );
	 gtk_box_pack_start( GTK_BOX( box->priv->image_box ), box->priv->image, FALSE, TRUE, 12 );
	 break;
	 
      case PROP_TEXT:
	 box->priv->content = g_value_get_object( value );
	 gtk_box_pack_start( GTK_BOX( box->priv->content_box ), box->priv->content, FALSE, TRUE, 12 );
	 break;
	 
      default: 
	 G_OBJECT_WARN_INVALID_PROPERTY_ID( obj, property_id, pspec );
	 break;
   }
}

static void ucview_info_box_get_property( GObject *obj, guint property_id, GValue *value, GParamSpec *pspec )
{
   UCViewInfoBox *box = UCVIEW_INFO_BOX( obj );
   
   switch( property_id )
   {
      case PROP_IMAGE:
	 g_value_set_object( value, box->priv->image );
	 break;
	 
      case PROP_TEXT:
	 g_value_set_object( value, box->priv->content );
	 break;
      
      default: 
	 G_OBJECT_WARN_INVALID_PROPERTY_ID( obj, property_id, pspec );
	 break;
   }
}

static gboolean paint_ucview_info_box( GtkWidget *widget, 
				GdkEventExpose *event, 
				gpointer user_data )
{
   gtk_paint_flat_box( widget->style, 
		       widget->window,
		       GTK_STATE_NORMAL,
		       GTK_SHADOW_OUT,
		       NULL,
		       widget,
		       "tooltip",
		       widget->allocation.x + 1,
		       widget->allocation.y + 1,
		       widget->allocation.width - 2,
		       widget->allocation.height - 2);
   
   return FALSE;		      
}

static void style_set( GtkWidget *widget, GtkStyle  *prev_style)
{
/*    GtkTooltips *tooltips; */
/*    GtkStyle *style; */
   
/*    UCViewInfoBox *box = UCVIEW_INFO_BOX( widget ); */
   
/*    if( box->priv->changing_style ) */
/*       return; */
	
/*    tooltips = gtk_tooltips_new( ); */
/*    g_object_ref_sink( tooltips ); */
   
/*    gtk_tooltips_force_window( tooltips ); */
/*    gtk_widget_ensure_style( tooltips->tip_window ); */
/*    style = gtk_widget_get_style (tooltips->tip_window); */
   
/*    box->priv->changing_style = TRUE; */
/*    gtk_widget_set_style (GTK_WIDGET (widget), style); */
/*    box->priv->changing_style = FALSE;	 */
   
/*    g_object_unref (tooltips); */
}

static ResponseData *get_response_data( GtkWidget *widget, gboolean create )
{
   ResponseData *rd;

   rd = g_object_get_data( G_OBJECT( widget ), "info-box-response-data" );
   
   if( ( rd == NULL ) && 
       ( create ) )
   {
      rd = g_new( ResponseData, 1 );
      
      g_object_set_data_full( G_OBJECT( widget ), "info-box-response-data", rd, g_free );
   }
   
   return rd;
}

static void action_widget_activated( GtkWidget *widget, UCViewInfoBox *box )
{
   ResponseData *rd;
   
   rd = get_response_data( widget, FALSE );
   
   ucview_info_box_response( box, rd ? rd->response_id : GTK_RESPONSE_NONE );
}


void ucview_info_box_add_action_widget( UCViewInfoBox  *box, GtkWidget *widget, gint response_id )
{
   ResponseData *rd;
   guint signal_id;

   g_return_if_fail( IS_UCVIEW_INFO_BOX( box ) );
   g_return_if_fail( GTK_IS_WIDGET( widget ) );

   rd = get_response_data( widget, TRUE );
   rd->response_id = response_id;
   
   if( GTK_IS_BUTTON( widget ) )
   {
      signal_id = g_signal_lookup( "clicked", GTK_TYPE_BUTTON );
   }
   else
   {
      signal_id = GTK_WIDGET_GET_CLASS( widget )->activate_signal;
   }
   
   if( signal_id )
   {
      GClosure *closure;
      
      closure = g_cclosure_new_object( G_CALLBACK( action_widget_activated ), G_OBJECT( box ) );
      
      g_signal_connect_closure_by_id( widget, signal_id, 0, closure, FALSE );
   }
   else
   {
      g_warning( "UCViewInfoBox: add_action_widget: Item not activatable" );
   }
   
   if( response_id != GTK_RESPONSE_HELP )
   {
      gtk_box_pack_start( GTK_BOX( box->priv->action_box ), widget, FALSE, FALSE, 12 );
   }
   else
   {
      gtk_box_pack_end( GTK_BOX( box->priv->action_box ), widget, FALSE, FALSE, 12 );
   }   
}

static void ucview_info_box_response( UCViewInfoBox *box, gint response_id )
{
   g_return_if_fail( IS_UCVIEW_INFO_BOX( box ) );
   
   g_signal_emit( box, signals[ RESPONSE ], 0, response_id );
}
