/*
** ucview.h
** 
** Made by Arne Caspari
** Login   <arne@arne-laptop>
** 
** Started on  Mon Feb  5 07:47:50 2007 Arne Caspari
** Last update Fri Mar 16 18:37:09 2007 Arne Caspari
*/

#ifndef   	UCVIEW_H_
# define   	UCVIEW_H_

#include <unicap.h>

typedef gboolean (*UCViewSaveImageFileHandler)(unicap_data_buffer_t *buffer, gchar *path, gchar *filetype, gpointer user_ptr, GError **err);


#define UCVIEW_ERROR g_quark_from_string( "UCView Error" )


#endif 	    /* !UCVIEW_H_ */
