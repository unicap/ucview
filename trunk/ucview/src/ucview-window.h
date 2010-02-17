#ifndef __UCVIEW_WINDOW_H__
#define __UCVIEW_WINDOW_H__

#include <unicap.h>
#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <glade/glade.h>
#include <gconf/gconf-client.h>
#include <ucil.h>

#include <time.h>
#include <sys/time.h>

#include <ucview/ucview.h>

#define UCVIEW_GCONF_DIR "/apps/ucview"
#define UCVIEW_PLUGIN_GCONF_DIR "plugins"

GType ucview_window_get_type( void );

#define UCVIEW_WINDOW_TYPE            (ucview_window_get_type())
#define UCVIEW_WINDOW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), UCVIEW_WINDOW_TYPE, UCViewWindow))
#define UCVIEW_WINDOW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), UCVIEW_WINDOW_TYPE, UCViewWindowClass))
#define IS_UCVIEW_WINDOW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), UCVIEW_WINDOW_TYPE))
#define IS_UCVIEW_WINDOW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), UCVIEW_WINDOW_TYPE))
#define UCVIEW_WINDOW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), UCVIEW_WINDOW_TYPE, UCViewWindowClass))

typedef struct _UCViewWindow UCViewWindow;
typedef struct _UCViewWindowClass UCViewWindowClass;
typedef struct _UCViewWindowPrivate UCViewWindowPrivate;

extern guint ucview_signals[];


struct _UCViewWindow
{
      GtkWindow window;

      /*< protected >*/
      GladeXML *glade;

      volatile gboolean still_image;
      gchar still_image_path[1024];
      gchar still_image_file_type[16];

      GtkWidget *vtoolbarwindow;
      GtkWidget *display;
      GtkWidget *display_window;
      GtkWidget *display_ebox;
      GtkWidget *settings_dialog;
      GtkWidget *property_dialog;
      GtkWidget *video_processing_progressbar;
      GtkWidget *sidebar;

      volatile gboolean prevent_updates;
      
      gdouble video_processing_fract;

      guint gconf_cnxn_id;

      GMutex *mutex;

      GConfClient *client;

      gboolean size_restored;
      gboolean is_fullscreen;

      unicap_handle_t device_handle;
      unicap_format_t format;
      unicap_data_buffer_t still_image_buffer;

      gboolean line_start_point_set;
      gint line_points[4];

      volatile gboolean time_lapse;
      gboolean time_lapse_first_frame;
      gint time_lapse_mode;
      GTimer *time_lapse_timer;
      GTimer *time_lapse_total_timer;
      guint time_lapse_delay;
      guint time_lapse_total_time;
      guint time_lapse_total_frames;
      guint time_lapse_frames;
      volatile gboolean record;

      volatile gboolean clipboard_copy_image;

      ucil_video_file_object_t *video_object;
      gchar *last_video_file;
      gchar *video_filename;
      unicap_handle_t video_file_handle;


      gint fullscreen_timeout;

      GList *modules;

      /*< private >*/
      UCViewWindowPrivate *priv;

};

struct _UCViewWindowClass
{
      GtkWindowClass parent_class;

      void (*ucview_window)(UCViewWindow *window);

      GConfClient *gconf_client;
};

enum
{
   UCVIEW_TIME_LAPSE_VIDEO = 0, 
   UCVIEW_TIME_LAPSE_IMAGE_SEQUENCE
};

enum
{
   UCVIEW_STOP_CONDITION_FRAMES = 0,
   UCVIEW_STOP_CONDITION_TIME
};

enum
{
   UCVIEW_DISPLAY_IMAGE_SIGNAL = 0, 
   UCVIEW_SAVE_IMAGE_SIGNAL, 
   UCVIEW_IMAGE_FILE_CREATED_SIGNAL, 
   UCVIEW_RECORD_START_SIGNAL, 
   UCVIEW_TIME_LAPSE_START_SIGNAL,
   UCVIEW_VIDEO_FILE_CREATED_SIGNAL, 
   UCVIEW_VIDEO_FORMAT_CHANGED_SIGNAL,
   UCVIEW_IMAGE_FILE_TYPE_ADDED_SIGNAL, 
   UCVIEW_IMAGE_FILE_TYPE_REMOVED_SIGNAL, 
   UCVIEW_LAST_SIGNAL
};


GtkWidget *ucview_window_new( unicap_handle_t handle );
void ucview_window_set_info_box( UCViewWindow *window, GtkWidget *info_box );
void ucview_window_clear_info_box( UCViewWindow *window );
void ucview_window_set_fullscreen( UCViewWindow *window, gboolean fullscreen );
void ucview_window_get_video_format( UCViewWindow *window, unicap_format_t *format );
unicap_status_t ucview_window_set_video_format( UCViewWindow *window, unicap_format_t *format );
void ucview_window_set_handle( UCViewWindow *window, unicap_handle_t handle );
unicap_status_t ucview_window_start_live( UCViewWindow *window );
void ucview_window_stop_live( UCViewWindow *window );
void ucview_window_enable_fps_display( UCViewWindow *ucv, gboolean enable );
gboolean ucview_window_save_still_image( UCViewWindow *window, const gchar *_filename, const gchar *_filetype, GError **error );
void ucview_window_register_image_file_type( UCViewWindow *window, gchar *filetype, gchar *suffix, UCViewSaveImageFileHandler handler, gpointer user_ptr );
void ucview_window_deregister_image_file_type( UCViewWindow *window, gchar *filetype );

GtkUIManager *ucview_window_get_ui_manager( UCViewWindow *ucv );
GtkWidget *ucview_window_get_statusbar( UCViewWindow *ucv );


#endif//__UCVIEW_WINDOW_H__
