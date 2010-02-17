#ifndef __ucvidplay_h__
#define __ucvidplay_h__


#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <gtk/gtkwindow.h>
#include <gdk/gdkx.h>
#include <gst/gst.h>
#include <gst/interfaces/xoverlay.h>


#include <unicap.h>
#include <unicap_status.h>


G_BEGIN_DECLS

#define UC_TYPE_VIDEO_PLAYER            (uc_video_player_get_type())
#define UC_VIDEO_PLAYER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), UC_TYPE_VIDEO_PLAYER, UCVideoPlayer))
#define UC_VIDEO_PLAYER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), UC_VIDEO_PLAYER_TYPE, UCVideoPlayerClass))
#define UC_IS_VIDEO_PLAYER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), UC_VIDEO_PLAYER_TYPE))
#define UC_IS_VIDEO_PLAYER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), UC_VIDEO_PLAYER_TYPE))




typedef struct _UCVideoPlayer       UCVideoPlayer;
typedef struct _UCVideoPlayerClass  UCVideoPlayerClass;


struct _UCVideoPlayer 
{
      GtkWindow window;

      GtkWidget *button_rewind;
      GtkWidget *button_play;
      GtkWidget *button_stop;
      
      GtkWidget *time_label;

      GtkWidget *drawing_area;

      GstElement *pipeline;

   guint update_pos_id;
      
      
};


struct _UCVideoPlayerClass
{
      GtkWindowClass parent_class;
};


GType      uc_video_player_get_type        ( void );

GtkWidget *uc_video_player_new ( void );
gboolean   uc_video_player_play_file( UCVideoPlayer *player, gchar *path );


G_END_DECLS

#endif //__ucvidplay_h__
