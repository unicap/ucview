/*
** ucview-about-dialog.c
** 
** Made by (Arne Caspari)
** Login   <arne@localhost>
** 
** Started on  Fri Jul 13 07:20:55 2007 Arne Caspari
*/

#include "config.h"
#include "ucview-about-dialog.h"
#include "ucview.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>


static const gchar *authors[] = {"Arne Caspari <arne@unicap-imaging.org>", NULL};

static const gchar comments[] = "An easy to use video capture application";

static const gchar license[] = 
"This program is free software; you can redistribute it and/or modify\n" 
"it under the terms of the GNU General Public License as published by\n" 
"the Free Software Foundation; either version 2 of the License, or\n" 
"(at your option) any later version.\n"
"\n"
"This program is distributed in the hope that it will be useful,\n"
"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
"GNU General Public License for more details.\n"
"\n"
"You should have received a copy of the GNU General Public License\n"
"along with this program; if not, write to the Free Software\n"
"Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA\n";

static const gchar copyright[] = "Copyright (C) 2007-2009 Arne Caspari";

static const gchar version[] = UCVIEW_VERSION_STRING;

static const gchar website[] = "http://www.unicap-imaging.org";


void ucview_about_dialog_show_action( GtkAction *action, UCViewWindow *ucv )
{
   gtk_show_about_dialog( GTK_WINDOW( ucv ), 
			  "authors", authors,
			  "comments", comments, 
			  "license", license, 
			  "copyright", copyright, 
			  "version", version,
			  "website", website, 
			  NULL );
}

