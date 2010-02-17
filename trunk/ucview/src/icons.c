/*
** icons.c
** 
** Made by (Arne Caspari)
** Login   <arne@localhost>
** 
** Started on  Fri Jul 13 18:01:52 2007 Arne Caspari
*/

#include "config.h"
#include "ucview-window.h"
#include "icons.h"

#include <gtk/gtk.h>
#include <glib/gprintf.h>
#include <glib/gi18n.h>

typedef struct 
{
      GtkStockItem stock_item;
      gchar *replacement;
} UCViewStockItem;

static UCViewStockItem items[] = 
{
   { { UCVIEW_STOCK_DEVICE_SETTINGS, N_("_Device Settings"), 0, 0, GETTEXT_PACKAGE }, GTK_STOCK_PROPERTIES }, 
   { { UCVIEW_STOCK_SAVE_STILL_IMAGE, N_("_Save Image"), 0, 0, GETTEXT_PACKAGE }, GTK_STOCK_SAVE }, 
   { { UCVIEW_STOCK_RECORD_VIDEO, N_("_Record Video"), 0, 0, GETTEXT_PACKAGE }, GTK_STOCK_MEDIA_RECORD }, 
   { { UCVIEW_STOCK_PLAY_VIDEO, N_("_Play Video"), 0, 0, GETTEXT_PACKAGE }, GTK_STOCK_MEDIA_PLAY }, 
   { { UCVIEW_STOCK_PAUSE, N_("P_ause"), 0, 0, GETTEXT_PACKAGE }, GTK_STOCK_MEDIA_PAUSE }, 
   { { UCVIEW_STOCK_PREFERENCES, N_("Preferences"), 0, 0, GETTEXT_PACKAGE }, GTK_STOCK_PREFERENCES }, 
   { { UCVIEW_STOCK_FULLSCREEN, N_("Fullscreen"), 0, 0, GETTEXT_PACKAGE }, GTK_STOCK_FULLSCREEN }, 
   { { UCVIEW_STOCK_LEAVE_FULLSCREEN, N_("Leave Fullscreen"), 0, 0, GETTEXT_PACKAGE }, GTK_STOCK_LEAVE_FULLSCREEN }, 
};

static gboolean add_icon_source( GtkIconSource *source, GtkIconSet *set, gchar *filename, GtkIconSize size )
{
   GtkSettings *settings;
   GdkPixbuf *pixbuf;
   gchar *path;
   gchar *theme_path;
   gchar *theme;
   gchar size_dir[32];
   gboolean wildcarded = FALSE;
   GConfClient *client;

   settings = gtk_settings_get_default();
   client = gconf_client_get_default();

   theme = gconf_client_get_string( client, UCVIEW_GCONF_DIR "/icon-theme-name", NULL );
   if( !theme )
   {
      g_object_get( G_OBJECT(settings), "gtk-icon-theme-name", &theme, NULL );
   }
   theme_path = g_build_path( G_DIR_SEPARATOR_S, UCVIEW_ICONDIR, theme, NULL );
   if( !g_file_test( theme_path, G_FILE_TEST_IS_DIR ) )
   {
      g_free( theme );
      g_free( theme_path );
      theme = g_strdup( "hicolor" );
      theme_path = g_build_path( G_DIR_SEPARATOR_S, UCVIEW_ICONDIR, theme, NULL );
   }
   
   g_free( theme_path );
   g_object_unref( client );
   
   switch( size )
   {
      case GTK_ICON_SIZE_LARGE_TOOLBAR:
	 g_sprintf( size_dir, "%dx%d", 48, 48 );
	 wildcarded = TRUE;
	 break;
	 
      case GTK_ICON_SIZE_SMALL_TOOLBAR:
	 g_sprintf( size_dir, "%dx%d", 24, 24 );
	 break;
	 
      case GTK_ICON_SIZE_MENU:
	 g_sprintf( size_dir, "%dx%d", 16, 16 );
	 break;

     default:
	 g_sprintf( size_dir, "%dx%d", 32, 32 );
	 break;
   }

   path = g_build_path( G_DIR_SEPARATOR_S, UCVIEW_ICONDIR, theme, size_dir, filename, NULL );
   
   if( wildcarded && !g_file_test( path, G_FILE_TEST_EXISTS ) )
   {
      // Exception for Asus icons
      g_free( path );
      g_sprintf( size_dir, "%dx%d", 41, 41 );
      path = g_build_path( G_DIR_SEPARATOR_S, UCVIEW_ICONDIR, theme, size_dir, filename, NULL );
   }

   g_free( theme );

   if( !g_file_test( path, G_FILE_TEST_EXISTS ) )
   {
      g_free( path );
      return FALSE;
   }
       


   pixbuf = gdk_pixbuf_new_from_file( path, NULL );
   g_free( path );

   if( pixbuf )
   {
      
      gtk_icon_source_set_pixbuf( source, pixbuf );
      gtk_icon_source_set_size_wildcarded( source, wildcarded );
      gtk_icon_source_set_size( source, GTK_ICON_SIZE_LARGE_TOOLBAR );
      gtk_icon_set_add_source( set, source );
      g_object_unref( pixbuf );
   }

   return TRUE;
}



void icons_add_stock_items( void )
{
   static gboolean items_added = FALSE;
   GtkIconFactory *factory;
   int i;

   if( items_added )
   {
      return;
   }
   items_added = TRUE;
   
   for( i = 0; i < G_N_ELEMENTS( items ); i++ )
   {
      gtk_stock_add_static( &items[i].stock_item, 1 );
   }

   factory = gtk_icon_factory_new();
   
   for( i = 0; i < G_N_ELEMENTS( items ); i++ )
   {
      GtkIconSet *set;
      GtkIconSource *source;
      gchar *filename;

      set = gtk_icon_set_new();
      source = gtk_icon_source_new();
      
      filename = g_strconcat( items[i].stock_item.stock_id, ".png", NULL );

      if( !add_icon_source( source, set, filename, GTK_ICON_SIZE_LARGE_TOOLBAR ) &&
	  !add_icon_source( source, set, filename, GTK_ICON_SIZE_SMALL_TOOLBAR ) &&
	  !add_icon_source( source, set, filename, GTK_ICON_SIZE_MENU ) )
      {
	 gtk_icon_set_unref( set );
	 set = gtk_icon_factory_lookup_default( items[i].replacement );
      }
      
      gtk_icon_factory_add( factory, items[i].stock_item.stock_id, set );
      gtk_icon_set_unref( set );
      gtk_icon_source_free( source );
   }
   
   gtk_icon_factory_add_default( factory );
   g_object_unref( factory );
}
