#include<gtk/gtk.h>
#include <sys/types.h>
#include <unistd.h>//sleep
//#include <string.h>
#include "cam.h"
#include "gallery.h"
#include "filters.h"
GtkWidget 		*window;
GtkWidget 		*rootLayout;
GtkWidget 		*captureButton;
GtkWidget 		*imageBoxLayout;
GtkWidget 		*imageBox;
GtkBuilder		*builder;
GtkTextBuffer 	*deviceInfo_textbuffer;
GtkWidget		*galleryFlowBox;

//https://stackoverflow.com/questions/60949269/executing-gtk-functions-from-other-threads
static GMutex camera_access_mutex; //shared between display and capture button
/*
*
*
*/
gboolean updateImage(gpointer buffer){

	if (imageBox!=NULL || imageBoxLayout!=NULL){
		
		//1. move data to gdkpixbuff struct
		GdkPixbuf* pixbuff=gdk_pixbuf_new_from_data (
			  (uint8_t*)buffer,
			  GDK_COLORSPACE_RGB,
			  0,
			  8,//int bits_per_sample=
			  fmt.fmt.pix.width,
			  fmt.fmt.pix.height,
			  fmt.fmt.pix.width*3,
			  NULL,//GdkPixbufDestroyNotify destroy_fn=
			  NULL//gpointer destroy_fn_data
		);
		
		//2. scale gdkpixbuff iamge struct .ERROR-->Leads buffer overflow. Could not manage the struct
		/*
		pixbuff=gdk_pixbuf_scale_simple (
			  pixbuff,
			  gtk_widget_get_allocated_width(imageBoxLayout),
			  gtk_widget_get_allocated_height(imageBoxLayout),
			  GDK_INTERP_BILINEAR
		);
		*/
		//3. set data to GtkImage object
		gtk_image_set_from_pixbuf(GTK_IMAGE(imageBox), pixbuff);//set method changes imageBox ton new image  and stimulates widget
		
		}
		return G_SOURCE_REMOVE;
}

gboolean updateImageThread(gpointer args){
		
		g_mutex_lock (&camera_access_mutex);

		uint8_t* buff=get_RGB_buff();

		updateImage( buff);
		free(buff);
		
		g_mutex_unlock (&camera_access_mutex);
		
	return G_SOURCE_REMOVE;
}

gboolean captureImage(gpointer data){

	g_mutex_lock (&camera_access_mutex);
	
	char *text=data;
	text=dump_buffer_to_file(text);//get saved image name
	
	gallery__load_image(galleryFlowBox, text, 176, 176, 1);//adds image to galleryFlowBox
	
	g_mutex_unlock (&camera_access_mutex);
	
	return G_SOURCE_REMOVE;
}
void captureButtonClicked(GtkButton *b){
	//dumps camera buffer that comes from cam.h to file

	g_idle_add_full(
	G_PRIORITY_HIGH_IDLE,//function priority
	captureImage,
	"image_shot",
	NULL);//https://docs.gtk.org/glib/func.idle_add_full.html
	
}

static void app_activate (GApplication *app, gpointer user_data) {
	
	//gtk_init(&argc, &argv);//init gtk
	
	builder=gtk_builder_new_from_file("halocam.glade");
	
	window=GTK_WIDGET(gtk_builder_get_object(builder,"window"));
	rootLayout=GTK_WIDGET(gtk_builder_get_object(builder,"rootLayout"));
	imageBoxLayout=GTK_WIDGET(gtk_builder_get_object(builder,"imageBoxLayout"));
	imageBox=GTK_WIDGET(gtk_builder_get_object(builder,"imageBox"));
	captureButton=GTK_WIDGET(gtk_builder_get_object(builder,"captureButton"));
	deviceInfo_textbuffer=GTK_TEXT_BUFFER(gtk_builder_get_object(builder,"deviceInfo_textbuffer"));
	
	gtk_text_buffer_set_text(GTK_TEXT_BUFFER(deviceInfo_textbuffer),device_specs.text,device_specs.length);
	
	galleryFlowBox=GTK_WIDGET(gtk_builder_get_object(builder,"galleryFlowBox"));
	
	g_signal_connect(window,"destroy",G_CALLBACK(gtk_main_quit),NULL);
	
	g_signal_connect(imageBox,"draw",G_CALLBACK(updateImageThread),NULL);
	gtk_builder_connect_signals(builder,NULL);
	//---
	gallery__load_all_images(galleryFlowBox, "../images/");
	gtk_window_set_application (GTK_WINDOW (window), GTK_APPLICATION (app));
	g_object_unref(builder);
	//---
	gtk_widget_show(window);
}

int main(int argc, char **argv){
	activate(); //initialize camera

	GtkApplication *app;
    int stat;

    app = gtk_application_new ("org.halosoft.halocam", G_APPLICATION_HANDLES_OPEN);
    //g_signal_connect (app, "startup", G_CALLBACK (castart), NULL);
    g_signal_connect (app, "activate", G_CALLBACK (app_activate), NULL);
    stat = g_application_run (G_APPLICATION (app), argc, argv);
    g_object_unref (app);
    
	return stat;
}
