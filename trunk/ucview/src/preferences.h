/*
** preferences.h
** 
** Made by Arne Caspari
** Login   <arne@localhost>
** 
** Started on  Thu Jul 26 07:54:20 2007 Arne Caspari
** Last update Thu Jul 26 07:54:20 2007 Arne Caspari
*/

#ifndef   	PREFERENCES_H_
# define   	PREFERENCES_H_

#include "ucview-window.h"

void prefs_store_window_state( UCViewWindow *window );
gboolean prefs_restore_window_state( UCViewWindow *window );

#endif 	    /* !PREFERENCES_H_ */
