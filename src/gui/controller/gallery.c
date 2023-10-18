#include<gtk/gtk.h>
#include<stdint.h>
#include "filters.h"
#include "laplace.h"	//laplace definitions
#include "gauss-blur.h"	//gaussian blurring definitions
#include "conv2d.h"		//convolve definitions
#include "normalize.h"	//convolve definitions
#include "pad.h"		//convolve definitions

GtkBuilder		*gallery__builder;
GtkWidget		*gallery__display_window;
GtkWidget 		*gallery__display_image;
GtkWidget 		*gallery__laplaceButton;
GtkWidget 		*gallery__gaussBlurButton;
GtkWidget 		*gallery__inverseButton;
GtkWidget 		*gallery__binaryButton;
GtkWidget 		*gallery__grayscaleButton;
GtkWidget 		*gallery__padButton;

void gallery__binaryButtonClicked(GtkButton *b);
int gallery__open_display_window( const char* file_path);
/*
*
*/
void imageButton_clicked(GtkButton *b){
		gallery__open_display_window( gtk_widget_get_tooltip_text(  GTK_WIDGET(b) ) );
}

void gallery__load_image(GtkWidget *galleryFlowBox, const char *file, int width, int height, gboolean preserve_aspect_ratio){
		GdkPixbuf* imgBuff=gdk_pixbuf_new_from_file_at_scale(
		file,
			width,
			height,
			preserve_aspect_ratio,		//preservfe aspect ratio
			NULL);
			
		GtkWidget *w=gtk_image_new_from_pixbuf(imgBuff);
		GtkWidget *button = gtk_button_new ();
		gtk_button_set_image (GTK_BUTTON (button), w);
		gtk_widget_set_tooltip_text(button,file);//keep file location as tooltip
		g_signal_connect(button, "clicked", G_CALLBACK(imageButton_clicked),NULL); 
		
		gtk_flow_box_insert(GTK_FLOW_BOX(galleryFlowBox), button,0);
		gtk_widget_show(button);
}

void gallery__load_all_images(GtkWidget *galleryFlowBox, const char *path){

	GDir *Dir=g_dir_open(path,0,NULL);
	
	const char* file;

	while( ( file=g_dir_read_name(Dir) )!=NULL ){

		char * extension=strrchr(file,'.');
		if ( extension!=NULL  && (strcmp( extension,".jpg" )==0) | (strcmp( extension,".jpeg" )==0) | (strcmp( extension,".png" )==0) ){
			
			char found_img[512];
			strcpy(found_img,path);
			strcat(found_img,file);
			
			int width=gtk_widget_get_allocated_width(galleryFlowBox)/5;
			if(width==0)
				width=176;
				
			gallery__load_image(galleryFlowBox, found_img, width, width,1);
			
		}
	}
	g_dir_close(Dir);
}

int gallery__open_display_window( const char* file_path){

	if (gallery__display_window!=NULL) {

		gtk_window_close (GTK_WINDOW(gallery__display_window));
	}
		
		gallery__builder=gtk_builder_new_from_file("../resources/view/gallery.glade");
	
		gallery__display_window=GTK_WIDGET(gtk_builder_get_object(gallery__builder,"gallery__display_window"));	
		gallery__laplaceButton=GTK_WIDGET(gtk_builder_get_object(gallery__builder,"laplaceButton"));	
		gallery__gaussBlurButton=GTK_WIDGET(gtk_builder_get_object(gallery__builder,"gaussBlurButton"));	
		gallery__inverseButton=GTK_WIDGET(gtk_builder_get_object(gallery__builder,"inverseButton"));	
		gallery__binaryButton=GTK_WIDGET(gtk_builder_get_object(gallery__builder,"binaryButton"));	
		gallery__grayscaleButton=GTK_WIDGET(gtk_builder_get_object(gallery__builder,"grayscaleButton"));	
		gallery__display_image=GTK_WIDGET(gtk_builder_get_object(gallery__builder,"gallery__display_image"));	
		gallery__padButton=GTK_WIDGET(gtk_builder_get_object(gallery__builder,"padButton"));

		GdkPixbuf* imgBuff=gdk_pixbuf_new_from_file_at_scale(
		file_path,
			600,
			450,
			1,		//preservfe aspect ratio
			NULL);
		gtk_image_set_from_pixbuf(GTK_IMAGE(gallery__display_image), imgBuff);
		
		g_signal_connect(gallery__display_window,"destroy",G_CALLBACK(gtk_window_close),NULL);
		g_signal_connect(gallery__binaryButton,"clicked",G_CALLBACK(gallery__binaryButtonClicked),NULL);
		g_signal_connect(gallery__inverseButton,"clicked",G_CALLBACK(gallery__binaryButtonClicked),NULL);
		g_signal_connect(gallery__grayscaleButton,"clicked",G_CALLBACK(gallery__binaryButtonClicked),NULL);
		g_signal_connect(gallery__gaussBlurButton,"clicked",G_CALLBACK(gallery__binaryButtonClicked),NULL);
		g_signal_connect(gallery__laplaceButton,"clicked",G_CALLBACK(gallery__binaryButtonClicked),NULL);
		g_signal_connect(gallery__padButton,"clicked",G_CALLBACK(gallery__binaryButtonClicked),NULL);
		//gtk_box_set_center_widget ( GTK_BOX(box), display_image );
	
		gtk_widget_show_all (gallery__display_window);
		
	return 0;
}

void gallery__binaryButtonClicked(GtkButton *b){
	//g_app_info_launch_default_for_uri("file:///",NULL,NULL);//open system-default image viewer
	const char *button_name=gtk_button_get_label(b);
	
	GdkPixbuf* imgBuff=gtk_image_get_pixbuf ( GTK_IMAGE(gallery__display_image) );
	int new_width=gdk_pixbuf_get_width(imgBuff), new_height=gdk_pixbuf_get_height(imgBuff);
	
	guchar* buff=gdk_pixbuf_get_pixels(imgBuff);
	
	if(strcmp(button_name,"binary")==0){
		im2bw(buff, gdk_pixbuf_get_byte_length(imgBuff) );
	}
	else if(strcmp(button_name,"grayscale")==0){
		im2gray(buff, gdk_pixbuf_get_byte_length(imgBuff) );
	}
	else if(strcmp(button_name,"inverse")==0){
		im2inverse(buff, gdk_pixbuf_get_byte_length(imgBuff) );
	}
	else if(strcmp(button_name,"gauss blur")==0){
		int fsize=5;
		float fsigma=1;
		int padsize=(fsize-1)/2;

		buff=pad(buff, gdk_pixbuf_get_width(imgBuff),gdk_pixbuf_get_height(imgBuff), padsize,padsize,padsize,padsize);
		buff=gaussBlur(buff, gdk_pixbuf_get_width(imgBuff)+padsize*2, gdk_pixbuf_get_height(imgBuff)+padsize*2, fsize,fsigma);
	}
	else if(strcmp(button_name,"laplace")==0){
		int fsize=3;
		int padsize=(fsize-1)/2;
		
		buff=pad(buff, gdk_pixbuf_get_width(imgBuff),gdk_pixbuf_get_height(imgBuff), padsize,padsize,padsize,padsize);
		float *lapl=laplacian(buff, gdk_pixbuf_get_width(imgBuff)+padsize*2, gdk_pixbuf_get_height(imgBuff)+padsize*2);
		
		buff=normalize(lapl, gdk_pixbuf_get_width(imgBuff), gdk_pixbuf_get_height(imgBuff));
	}
	else if(strcmp(button_name,"zero pad")==0){
		int fsize=3;
		int padsize=(fsize-1)/2;
		
		buff=pad(buff, gdk_pixbuf_get_width(imgBuff),gdk_pixbuf_get_height(imgBuff), padsize,padsize,padsize,padsize);
		
		new_width+=2*padsize;
		new_height+=2*padsize;
	}
	
	GdkPixbuf *newpix=gdk_pixbuf_new_from_data (
		  buff,
		  GDK_COLORSPACE_RGB,
		  0,
		  gdk_pixbuf_get_bits_per_sample(imgBuff),
		  new_width,
		  new_height,
		  new_width*3,
		  NULL,NULL
	);
	
	gtk_image_set_from_pixbuf(GTK_IMAGE(gallery__display_image), newpix);
}
