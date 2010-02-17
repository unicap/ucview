/*
** utils.c
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

#include "utils.h"


GtkWidget *ucview_gtk_button_new_with_stock_icon( const gchar *label, const gchar *stock_id )
{
   GtkWidget *button;
   
   button = gtk_button_new_with_mnemonic( label );
   gtk_button_set_image( GTK_BUTTON( button ), gtk_image_new_from_stock( stock_id, GTK_ICON_SIZE_BUTTON ) );
   
   return button;
}
