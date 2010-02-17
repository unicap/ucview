/*
** user_config.c
** 
*/

#include "user_config.h"
#include <unicap.h>
#include <unicapgtk.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <stdlib.h>
#include <string.h>

#include "worker.h"

static gchar *get_config_path( void )
{
   char *homepath;
   gchar *path;
   
   homepath = getenv( "HOME" );
   if( !homepath )
   {
      return NULL;
   }
   
   path = g_build_path( G_DIR_SEPARATOR_S, homepath, ".ucview", NULL );
   if( g_file_test( path, G_FILE_TEST_EXISTS ) )
   {
      if( !g_file_test( path, G_FILE_TEST_IS_DIR ) )
      {
	 g_warning( "%s is not a directory\n", path );
	 g_free( path );
	 return NULL;
      }
   }
   else
   {
      if( g_mkdir( path, 0755 ) )
      {
	 g_warning( "Failed to create '%s'\n", path );
	 g_free( path );
	 return NULL;
      }
   }

   return path;
}


unicap_handle_t user_config_try_restore( void )
{
   GKeyFile *keyfile = NULL;
   gchar *config_path;
   gchar *ini_path;
   unicap_handle_t handle = NULL;
   
   config_path = get_config_path();
   ini_path = g_build_path( G_DIR_SEPARATOR_S, config_path, "default.ini", NULL );
   g_free( config_path );
   
   keyfile = g_key_file_new( );

   if( !g_key_file_load_from_file( keyfile, 
				   ini_path, 
				   G_KEY_FILE_NONE, 
				   NULL ) )
   {
      g_free( ini_path );
      ini_path = g_build_path( G_DIR_SEPARATOR_S, UCVIEW_GLOBAL_CONFIGDIR, "default.ini", NULL );
      if( !g_key_file_load_from_file( keyfile, 
				      ini_path, 
				      G_KEY_FILE_NONE, 
				      NULL ) )
      {
	 g_warning( "failed\n" );
	 g_key_file_free( keyfile );
	 keyfile = NULL;
      }
   }

   g_free( ini_path );

   if( keyfile )
   {
      gchar *devid = NULL;
      gchar *vendor_name = NULL;
      gchar *model_name = NULL;
	 
      unicap_device_t devspec;
      unicap_device_t device;
      
      unicap_void_device( &devspec );
      
      devid = g_key_file_get_string( keyfile, "Device", "Identifier", NULL );
      if( devid )
      {
	 strcpy( devspec.identifier, devid );
      }
      else
      {
	 vendor_name = g_key_file_get_string( keyfile, "Device", "Vendor", NULL );
	 model_name = g_key_file_get_string( keyfile, "Device", "Model", NULL );
	 
	 if( vendor_name && model_name )
	 {
	    strncpy( device.vendor_name, vendor_name, sizeof( device.vendor_name ) - 1 );
	    strncpy( device.model_name, model_name, sizeof( device.model_name ) - 1 );
	 }
      }
      

      // Only call unicap_open if a matching device was found,
      // otherwise we would always open any device
      if( devid || ( vendor_name && model_name ) )
      {
	 if( SUCCESS( unicap_enumerate_devices( &devspec, &device, 0 ) ) )
	 {
	    if( SUCCESS( unicap_open( &handle, &device ) ) )
	    {
	       unicapgtk_load_device_state( handle, keyfile, UNICAPGTK_DEVICE_STATE_VIDEO_FORMAT | UNICAPGTK_DEVICE_STATE_PROPERTIES );
	    }
	 }
      }
      if( devid )
      {
	 g_free( devid );
      }
      if( vendor_name )
      {
	 g_free( vendor_name );
      }
      if( model_name )
      {
	 g_free( model_name );
      }
      
      g_key_file_free( keyfile );
   }

   return handle;
}

void user_config_store_default( unicap_handle_t handle )
{   
   gchar *config_path;
   gchar *ini_path;
   gboolean result;
   GKeyFile *keyfile;

   config_path = get_config_path();
   ini_path = g_build_path( G_DIR_SEPARATOR_S, config_path, "default.ini", NULL );
   g_free( config_path );

   keyfile = unicapgtk_save_device_state( handle, UNICAPGTK_DEVICE_STATE_VIDEO_FORMAT | UNICAPGTK_DEVICE_STATE_PROPERTIES );
   if( keyfile )
   {
      FILE *f;
      gchar *data;
      gsize size;
      
      data = g_key_file_to_data( keyfile, &size, NULL );

      f = fopen( ini_path, "w" );
      if( f )
      {
	 int ignore;
	 ignore = fwrite( data, size, 1, f );
	 fclose( f );

	 result = TRUE;
      }
      
      g_free( data );
      g_key_file_free( keyfile );
   }

   g_free( ini_path );
}

void user_config_store_device( unicap_handle_t handle )
{
   gchar *config_path;
   gchar *ini_path;
   gchar *file_name;
   GKeyFile *keyfile;
   unicap_device_t device;

   unicap_get_device( handle, &device );
   
   config_path = get_config_path();
   file_name = g_strconcat( "_", device.identifier, ".ini", NULL );
   ini_path = g_build_path( G_DIR_SEPARATOR_S, config_path, file_name, NULL );
   g_free( file_name );
   g_free( config_path );

   keyfile = unicapgtk_save_device_state( handle, UNICAPGTK_DEVICE_STATE_VIDEO_FORMAT | UNICAPGTK_DEVICE_STATE_PROPERTIES );
   if( keyfile )
   {
      FILE *f;
      gchar *data;
      gsize size;
      
      data = g_key_file_to_data( keyfile, &size, NULL );

      f = fopen( ini_path, "w" );
      if( f )
      {
	 int ignore;
	 ignore = fwrite( data, size, 1, f );
	 fclose( f );
      }
      
      g_free( data );
      g_key_file_free( keyfile );
   }

   g_free( ini_path );
}

void user_config_restore_device( unicap_handle_t handle )
{
   GKeyFile *keyfile;
   gchar *config_path;
   gchar *ini_path;
   gchar *file_name;
   unicap_device_t device;
   

   unicap_get_device( handle, &device );

   config_path = get_config_path();
   file_name = g_strconcat( "_", device.identifier, ".ini", NULL );
   ini_path = g_build_path( G_DIR_SEPARATOR_S, config_path, file_name, NULL );
   g_free( file_name );
   g_free( config_path );
   
   keyfile = g_key_file_new( );
   if( g_key_file_load_from_file( keyfile, 
				  ini_path, 
				  G_KEY_FILE_NONE, 
				  NULL ) )
   {
      unicapgtk_load_device_state( handle, keyfile, UNICAPGTK_DEVICE_STATE_PROPERTIES );
   }

   g_free( ini_path );
}
