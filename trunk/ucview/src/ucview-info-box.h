/*
** ucview-info-box.h
** 
** Made by Arne Caspari
** Login   <arne@localhost>
** 
** Started on  Wed Jul 18 17:21:48 2007 Arne Caspari
** Last update Wed Jul 18 17:21:48 2007 Arne Caspari
*/

#ifndef  __UCVIEW_INFO_BOX_H__
#define __UCVIEW_INFO_BOX_H__

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>


GType ucview_info_box_get_type( void );
GtkWidget *ucview_info_box_new( );


#define UCVIEW_INFO_BOX_TYPE              (ucview_info_box_get_type())
#define UCVIEW_INFO_BOX(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj), UCVIEW_INFO_BOX_TYPE, UCViewInfoBox))
#define UCVIEW_INFO_BOX_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass), UCVIEW_INFO_BOX_TYPE, UCViewInfoBoxClass))
#define IS_UCVIEW_INFO_BOX(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj), UCVIEW_INFO_BOX_TYPE))
#define IS_UCVIEW_INFO_BOX_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE((klass), UCVIEW_INFO_BOX_TYPE))
#define UCVIEW_INFO_BOX_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), UCVIEW_INFO_BOX_TYPE, UCViewInfoBoxClass))


typedef struct _UCViewInfoBox UCViewInfoBox;
typedef struct _UCViewInfoBoxClass UCViewInfoBoxClass;
typedef struct _UCViewInfoBoxPrivate UCViewInfoBoxPrivate;

typedef enum
{
   UCVIEW_INFO_BOX_RESPONSE_ACTION1 = 1,
   UCVIEW_INFO_BOX_RESPONSE_ACTION2 = 2,
   UCVIEW_INFO_BOX_RESPONSE_ACTION3 = 3,
   UCVIEW_INFO_BOX_RESPONSE_ACTION4 = 4,
} UCViewInfoBoxResponse;


struct _UCViewInfoBox
{
      GtkHBox parent;

      /*< private >*/
      UCViewInfoBoxPrivate *priv;
};

struct _UCViewInfoBoxClass
{
      GtkHBoxClass parent_class;
      void (* response)( UCViewInfoBox *box, gint response_id );
};


void ucview_info_box_add_action_widget( UCViewInfoBox  *box, GtkWidget *widget, gint response_id );


#endif//__UCVIEW_INFO_BOX_H__
