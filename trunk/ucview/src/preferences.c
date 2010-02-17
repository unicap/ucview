/*
** preferences.c
** 
** Made by (Arne Caspari)
** Login   <arne@localhost>
** 
** Started on  Thu Jul 26 07:26:21 2007 Arne Caspari
*/

#include "preferences.h"

#include <gtk/gtk.h>
#include <gconf/gconf-client.h>

#include "ucview.h"

void prefs_store_window_state( UCViewWindow *window )
{
   GdkWindowState state;
   gint x, y, w, h;
   
   state = gdk_window_get_state( GTK_WIDGET( window )->window );

   if( !( state & GDK_WINDOW_STATE_FULLSCREEN ) )
   {
      gtk_window_get_size( GTK_WINDOW( window ), &w, &h );
      gtk_window_get_position( GTK_WINDOW( window ), &x, &y );
   
      gconf_client_set_int( window->client, UCVIEW_GCONF_DIR "/window_state", state, NULL );
      gconf_client_set_int( window->client, UCVIEW_GCONF_DIR "/window_x", x, NULL );
      gconf_client_set_int( window->client, UCVIEW_GCONF_DIR "/window_y", y, NULL );
      gconf_client_set_int( window->client, UCVIEW_GCONF_DIR "/window_w", w, NULL );
      gconf_client_set_int( window->client, UCVIEW_GCONF_DIR "/window_h", h, NULL );
   }
}

gboolean prefs_restore_window_state( UCViewWindow *window )
{
   GdkWindowState state;
   gint x, y, w, h;
   GError *err = NULL;
   
   state = gconf_client_get_int( window->client, UCVIEW_GCONF_DIR "/window_state", &err );
   if( err )
   {
      return FALSE;
   }
   
   if( state & GDK_WINDOW_STATE_MAXIMIZED )
   {
      gtk_window_maximize( GTK_WINDOW( window ) );
   }
   else
   {
      x = gconf_client_get_int( window->client, UCVIEW_GCONF_DIR "/window_x", &err );
      y = gconf_client_get_int( window->client, UCVIEW_GCONF_DIR "/window_y", &err );
      w = gconf_client_get_int( window->client, UCVIEW_GCONF_DIR "/window_w", &err );
      h = gconf_client_get_int( window->client, UCVIEW_GCONF_DIR "/window_h", &err );

      if( ( w < 64 ) || ( h < 64 ) )
      {
	 return FALSE;
      }
      
      gtk_window_move( GTK_WINDOW( window ), x, y );
      gtk_window_set_default_size( GTK_WINDOW( window ), w, h );
   }
   

   return TRUE;
}
