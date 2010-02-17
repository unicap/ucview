/*
** worker.c
** 
** Made by (Arne Caspari)
** Login   <arne@localhost>
** 
** Started on  Sun Sep  2 21:00:02 2007 Arne Caspari
*/

#include "worker.h"
#include <pthread.h>
#include <gtk/gtk.h>
#include <unistd.h>

struct worker_data
{
      int finish;
      void *data;
      void *(*func)(void *);
};


static void *worker_thread( void *_data )
{
   struct worker_data *data = _data;
   void *ret;
   
   data->finish = 0;
   ret = data->func( data->data );
   data->finish = 1;
   
   return ret;
}


void *run_worker( void *(*func)(void *), void *arg )
{
   pthread_t thread;
   struct worker_data data;
   void *ret;
   
   data.data = arg;
   data.finish = 0;
   data.func = func;
   
   pthread_create( &thread, NULL, worker_thread, &data );

   while( !data.finish )
   {
      while( gtk_events_pending() )
      {
	 gtk_main_iteration();
      }
      
      usleep( 1000 );
   }

   pthread_join( thread, &ret );
   
   return ret;
}

