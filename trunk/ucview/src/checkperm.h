/*
** checkperm.h
** 
** Made by Arne Caspari
** Login   <arne@arne-laptop>
** 
** Started on  Tue Jan 30 07:28:09 2007 Arne Caspari
** Last update Tue Feb  6 07:29:58 2007 Arne Caspari
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

#include <gtk/gtk.h>

#ifndef   	CHECKPERM_H_
# define   	CHECKPERM_H_

gboolean check_unicap_env( GConfClient *client );


#endif 	    /* !CHECKPERM_H_ */
