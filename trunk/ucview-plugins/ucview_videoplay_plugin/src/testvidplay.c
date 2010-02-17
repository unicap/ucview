#include <gtk/gtk.h>
#include "ucvidplay.h"


int main( int argc, char **argv )
{
   GtkWidget *player;
   
   gst_init( &argc, &argv );
   gtk_init( &argc, &argv );

   if( argc < 2 )
   {
      g_printerr( "Missing video file name\n" );
      exit( 1 );
   }
   
   player = uc_video_player_new();
   g_signal_connect( player, "delete-event", gtk_main_quit, NULL );
   
   gtk_widget_show_all( player );
   uc_video_player_play_file( UC_VIDEO_PLAYER( player ), argv[1] );

   gtk_main();

   return 0;
   
   
   
}

