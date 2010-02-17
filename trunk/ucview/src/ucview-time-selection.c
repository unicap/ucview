/*
** ucview_time_selection.c
** 
** Made by (Arne Caspari)
** Login   <arne@localhost>
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

#include "ucview-time-selection.h"
#include <gtk/gtk.h>
#include <glib/gi18n.h>

static void ucview_time_selection_class_init( UCViewTimeSelectionClass *klass );
static void ucview_time_selection_init( UCViewTimeSelection *box );
static void ucview_time_selection_set_property( GObject *obj, guint property_id, const GValue *value, GParamSpec *pspec );
static void ucview_time_selection_get_property( GObject *obj, guint property_id, GValue *value, GParamSpec *pspec );
static void spin_button_changed_cb( GtkSpinButton *spinbutton, UCViewTimeSelection *ucview_time_selection );

#define UCVIEW_TIME_SELECTION_GET_PRIVATE( obj )( G_TYPE_INSTANCE_GET_PRIVATE( ( obj ), UCVIEW_TIME_SELECTION_TYPE, UCViewTimeSelectionPrivate ) )

struct _UCViewTimeSelectionPrivate
{
      GtkWidget *day_spin;
      GtkWidget *hour_spin;
      GtkWidget *minute_spin;
      GtkWidget *second_spin;
};


G_DEFINE_TYPE( UCViewTimeSelection, ucview_time_selection, GTK_TYPE_HBOX );


enum
{
   PROP_0, 
   PROP_IMAGE, 
   PROP_TEXT, 
};

enum
{
   CHANGED_SIGNAL, 
   LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];

static void ucview_time_selection_class_init( UCViewTimeSelectionClass *klass )
{
   GObjectClass *object_class = G_OBJECT_CLASS( klass );
   
   object_class->set_property = ucview_time_selection_set_property;
   object_class->get_property = ucview_time_selection_get_property;
   
   g_type_class_add_private( object_class, sizeof( UCViewTimeSelectionPrivate ) );   

   signals[ CHANGED_SIGNAL ] = g_signal_new( "changed", 
					     G_OBJECT_CLASS_TYPE( klass ), 
					     G_SIGNAL_RUN_LAST, 
					     0, NULL, NULL, 
					     g_cclosure_marshal_VOID__INT, 
					     G_TYPE_NONE, 1, 
					     G_TYPE_INT );
}

static void ucview_time_selection_init( UCViewTimeSelection *box )
{
   box->priv = UCVIEW_TIME_SELECTION_GET_PRIVATE( box );

   box->priv->day_spin = gtk_spin_button_new_with_range( 0.0, 31.0, 1.0 );
   gtk_box_pack_start( GTK_BOX( box ), box->priv->day_spin, FALSE, FALSE, 0 );
   gtk_box_pack_start( GTK_BOX( box ), gtk_label_new( _("days") ), FALSE, FALSE, 4 );
   box->priv->hour_spin = gtk_spin_button_new_with_range( 0.0, 23.0, 1.0 );
   gtk_box_pack_start( GTK_BOX( box ), box->priv->hour_spin, FALSE, FALSE, 0 );
   gtk_box_pack_start( GTK_BOX( box ), gtk_label_new( _("hours") ), FALSE, FALSE, 4 );
   box->priv->minute_spin = gtk_spin_button_new_with_range( 0.0, 59.0, 1.0 );
   gtk_box_pack_start( GTK_BOX( box ), box->priv->minute_spin, FALSE, FALSE, 0 );
   gtk_box_pack_start( GTK_BOX( box ), gtk_label_new( _("minutes") ), FALSE, FALSE, 4 );
   box->priv->second_spin = gtk_spin_button_new_with_range( 0.0, 59.0, 1.0 );
   gtk_box_pack_start( GTK_BOX( box ), box->priv->second_spin, FALSE, FALSE, 0 );
   gtk_box_pack_start( GTK_BOX( box ), gtk_label_new( _("seconds") ), FALSE, FALSE, 4 );

   g_signal_connect( box->priv->day_spin, "value-changed", (GCallback)spin_button_changed_cb, box );
   g_signal_connect( box->priv->hour_spin, "value-changed", (GCallback)spin_button_changed_cb, box );
   g_signal_connect( box->priv->minute_spin, "value-changed", (GCallback)spin_button_changed_cb, box );
   g_signal_connect( box->priv->second_spin, "value-changed", (GCallback)spin_button_changed_cb, box );

   return;
}

static void ucview_time_selection_set_property( GObject *obj, guint property_id, const GValue *value, GParamSpec *pspec )
{
/*    UCViewTimeSelection *box = UCVIEW_TIME_SELECTION( obj ); */
   
   switch( property_id )
   {
      default: 
	 G_OBJECT_WARN_INVALID_PROPERTY_ID( obj, property_id, pspec );
	 break;
   }
}

static void ucview_time_selection_get_property( GObject *obj, guint property_id, GValue *value, GParamSpec *pspec )
{
/*    UCViewTimeSelection *box = UCVIEW_TIME_SELECTION( obj ); */
   
   switch( property_id )
   {
      default: 
	 G_OBJECT_WARN_INVALID_PROPERTY_ID( obj, property_id, pspec );
	 break;
   }
}

static void spin_button_changed_cb( GtkSpinButton *spinbutton, UCViewTimeSelection *ucview_time_selection )
{
   guint time = ucview_time_selection_get_time( ucview_time_selection );
   
   g_signal_emit( ucview_time_selection, signals[ CHANGED_SIGNAL ], 0, time );
}

void ucview_time_selection_set_time( UCViewTimeSelection *ucview_time_selection, guint time )
{
   gint days;
   gint hours;
   gint minutes;
   gint seconds;
   
   time /= 1000;

   days = ( time / ( 60 * 60 * 24 ) ) % 31;
   hours = ( time / ( 60 * 60 ) ) % 24;
   minutes = ( time / ( 60 ) ) % 60;
   seconds = time % 60;

   gtk_spin_button_set_value( GTK_SPIN_BUTTON( ucview_time_selection->priv->day_spin ), days );
   gtk_spin_button_set_value( GTK_SPIN_BUTTON( ucview_time_selection->priv->hour_spin ), hours );
   gtk_spin_button_set_value( GTK_SPIN_BUTTON( ucview_time_selection->priv->minute_spin ), minutes );
   gtk_spin_button_set_value( GTK_SPIN_BUTTON( ucview_time_selection->priv->second_spin ), seconds );
}

guint ucview_time_selection_get_time( UCViewTimeSelection *ucview_time_selection )
{
   gint days;
   gint hours;
   gint minutes;
   gint seconds;
   guint time;
   
   days = gtk_spin_button_get_value( GTK_SPIN_BUTTON( ucview_time_selection->priv->day_spin ) );
   hours = gtk_spin_button_get_value( GTK_SPIN_BUTTON( ucview_time_selection->priv->hour_spin ) );
   minutes = gtk_spin_button_get_value( GTK_SPIN_BUTTON( ucview_time_selection->priv->minute_spin ) );
   seconds = gtk_spin_button_get_value( GTK_SPIN_BUTTON( ucview_time_selection->priv->second_spin ) );

   time = seconds + ( minutes * 60 ) + ( hours * 60 * 60 ) + ( days * 60 * 60 * 24 );
   time *= 1000;

   return time;
}

   


GtkWidget *ucview_time_selection_new( guint time )
{
   UCViewTimeSelection *ucview_time_selection = ( UCViewTimeSelection * )g_object_new( UCVIEW_TIME_SELECTION_TYPE, NULL );

   ucview_time_selection_set_time( ucview_time_selection, time );

   return (GtkWidget*)ucview_time_selection;
}
