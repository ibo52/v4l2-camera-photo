#ifndef halocam_gui_gallery_
#define halocam_gui_gallery_
#include<stdint.h>
#include<gtk/gtk.h>

void gallery__load_image(GtkWidget *galleryFlowBox, const char *file, int width, int height,gboolean preserve_aspect_ratio);
void gallery__load_all_images(GtkWidget *galleryFlowBox, const char *path);
int gallery__open_display_window( const char* window_name);
#endif
