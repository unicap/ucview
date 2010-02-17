/*
** ucview-time-selection.h
** 
** Made by Arne Caspari
** Login   <arne@localhost>
** 
** Started on  Wed Jul 18 17:21:48 2007 Arne Caspari
** Last update Wed Jul 18 17:21:48 2007 Arne Caspari
*/

#ifndef   	UCVIEW_TIME_SELECTION_H_
# define   	UCVIEW_TIME_SELECTION_H_

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>


GType ucview_time_selection_get_type( void );
GtkWidget *ucview_time_selection_new( );


#define UCVIEW_TIME_SELECTION_TYPE              (ucview_time_selection_get_type())
#define UCVIEW_TIME_SELECTION(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj), UCVIEW_TIME_SELECTION_TYPE, UCViewTimeSelection))
#define UCVIEW_TIME_SELECTION_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass), UCVIEW_TIME_SELECTION_TYPE, UCViewTimeSelectionClass))
#define IS_UCVIEW_TIME_SELECTION(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj), UCVIEW_TIME_SELECTION_TYPE))
#define IS_UCVIEW_TIME_SELECTION_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE((klass), UCVIEW_TIME_SELECTION_TYPE))
#define UCVIEW_TIME_SELECTION_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), UCVIEW_TIME_SELECTION_TYPE, UCViewTimeSelectionClass))


typedef struct _UCViewTimeSelection UCViewTimeSelection;
typedef struct _UCViewTimeSelectionClass UCViewTimeSelectionClass;
typedef struct _UCViewTimeSelectionPrivate UCViewTimeSelectionPrivate;

struct _UCViewTimeSelection
{
      GtkHBox parent;

      /*< private >*/
      UCViewTimeSelectionPrivate *priv;
};

struct _UCViewTimeSelectionClass
{
      GtkHBoxClass parent_class;
      void (* response)( UCViewTimeSelection *box, gint response_id );
};


GtkWidget *ucview_time_selection_new( guint time );
void ucview_time_selection_set_time( UCViewTimeSelection *ucview_time_selection, guint time );
guint ucview_time_selection_get_time( UCViewTimeSelection *ucview_time_selection );


#endif 	    /* !UCVIEW_TIME_SELECTION_H_ */
