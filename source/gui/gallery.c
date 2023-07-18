#include<gtk/gtk.h>
#include<stdint.h>
//#include <string.h>

void gallery__load_image(GtkWidget *galleryFlowBox, const char *file, int width, int height, gboolean preserve_aspect_ratio){
		GdkPixbuf* imgBuff=gdk_pixbuf_new_from_file_at_scale(
		file,
			width,
			height,
			preserve_aspect_ratio,		//preservfe aspect ratio
			NULL);
			
		GtkWidget *w=gtk_image_new_from_pixbuf(imgBuff);
			
		gtk_flow_box_insert(GTK_FLOW_BOX(galleryFlowBox), w,0);
		gtk_widget_show(w);
}

void gallery__load_all_images(GtkWidget *galleryFlowBox, const char *path){

	GDir *Dir=g_dir_open(path,0,NULL);
	
	const char* file;

	while( ( file=g_dir_read_name(Dir) )!=NULL ){

		char * extension=strrchr(file,'.');
		if ( extension!=NULL  && strcmp( extension,".jpg" )==0 ){
			
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
