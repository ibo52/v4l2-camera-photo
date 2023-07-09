#include<gtk/gtk.h>
#include <sys/types.h>
#include <unistd.h>//sleep
#include <semaphore.h>
#include <sys/mman.h>
#include <fcntl.h>

#include "../../include/cam.h"
#define WIDTH 320
#define HEIGHT 240

GtkWidget 		*window;
GtkWidget 		*rootLayout;
GtkWidget 		*captureButton;
GtkWidget 		*imageBoxLayout;
GtkWidget 		*imageBox;
GtkBuilder		*builder;

void* updateImageThread(void *);
void updateImage();



int main(int argc, char **argv){

	activate();
	
	gtk_init(&argc, &argv);//init gtk
	
	builder=gtk_builder_new_from_file("halocam.glade");
	
	window=GTK_WIDGET(gtk_builder_get_object(builder,"window"));
	rootLayout=GTK_WIDGET(gtk_builder_get_object(builder,"rootLayout"));
	imageBoxLayout=GTK_WIDGET(gtk_builder_get_object(builder,"imageBoxLayout"));
	imageBox=GTK_WIDGET(gtk_builder_get_object(builder,"imageBox"));
	captureButton=GTK_WIDGET(gtk_builder_get_object(builder,"captureButton"));
	
	g_signal_connect(window,"destroy",G_CALLBACK(gtk_main_quit),NULL);
	
	g_thread_new("updateImageThread",updateImageThread,NULL);
	
	gtk_builder_connect_signals(builder,NULL);

	gtk_widget_show(window);
	gtk_main();

	return 0;
}

void *updateImageThread(void* args){

	while(1){
		uint8_t* buff=get_RGB_buff();
		updateImage(buff);
		free(buff);
		//sleep(1);
	}
	g_thread_exit(NULL);
}
void updateImage(uint8_t* buffer){

	if (imageBox!=NULL || imageBoxLayout!=NULL){
		
		//1. move data to gdkpixbuff struct
		GdkPixbuf* pixbuff=gdk_pixbuf_new_from_data (
			  buffer,
			  GDK_COLORSPACE_RGB,
			  0,
			  8,//int bits_per_sample=
			  fmt.fmt.pix.width,
			  fmt.fmt.pix.height,
			  fmt.fmt.pix.width*3,
			  NULL,//GdkPixbufDestroyNotify destroy_fn=
			  NULL//gpointer destroy_fn_data
		);
		
		//2. scale gdkpixbuff iamge struct
		/*
		GdkPixbuf *scaled_pixbuff=gdk_pixbuf_scale_simple (
			  pixbuff,
			  gtk_widget_get_allocated_width(imageBoxLayout),
			  gtk_widget_get_allocated_height(imageBoxLayout),
			  GDK_INTERP_BILINEAR
		);
		*/
		//3. set data to GtkImage object
		gtk_image_set_from_pixbuf(GTK_IMAGE(imageBox), pixbuff);//set method changes imageBox ton new image  and stimulates widget

		}
}

void captureButtonClicked(GtkButton *b){
	printf("button click\n");//updateImage();
}
