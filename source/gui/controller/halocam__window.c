#include<gtk/gtk.h>
#include <sys/types.h>
#include <unistd.h>//sleep
//#include <string.h>
#include "cam.h"
#include "gallery.h"
#include "filters.h"
#include "about-dialog.h"
#include "camera_settings-dialog.h"
#include "halocam__window.h"

//---CAMERA PAGE WİDGETS---
GtkWidget 		*window;
GtkWidget 		*rootLayout;	//MAİN LAYOUT OF WİNDOW
GtkWidget 		*captureButton;
GtkWidget 		*imageBoxLayout;
GtkWidget 		*imageBox;
GtkBuilder		*builder;
GtkWidget		*galleryFlowBox;

//---INFO PAGE WİDGETS---
GtkWidget 		*info__driver_label;
GtkWidget 		*info__card_label;
GtkWidget 		*info__bus_label;
GtkWidget 		*info__version_label;
GtkWidget 		*info__caps_label;

GtkWidget 		*info__buff_width;
GtkWidget 		*info__buff_height;
GtkWidget 		*info__buff_format;
GtkWidget 		*info__buff_colorspace;
GtkWidget		*info__device_path;
GtkWidget		*info__caps_listBox;
GtkWidget		*info__caps_extra_field;
//---INFO PAGE WİDGETS---

static void parse_caps(uint32_t cap, GtkBox* containerBox){
	//https://www.kernel.org/doc/html/v4.9/media/uapi/v4l/vidioc-querycap.html#id1
	GtkWidget *temp;
	
	PangoAttrList *baseAttrs = pango_attr_list_new();//text formatting
	pango_attr_list_insert(baseAttrs, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
	pango_attr_list_insert(baseAttrs, pango_attr_size_new(14*PANGO_SCALE));
	pango_attr_list_insert(baseAttrs, pango_attr_style_new(PANGO_STYLE_ITALIC));
	
	PangoAttrList* Attrs=pango_attr_list_copy(baseAttrs);
	pango_attr_list_insert(Attrs, pango_attr_foreground_new(0,32501,42501));
	
	if( cap & V4L2_CAP_VIDEO_CAPTURE ){
		temp=gtk_label_new("Video Capture(single-planar API)");
		gtk_label_set_attributes(GTK_LABEL(temp),Attrs);
		gtk_widget_set_halign (temp, GTK_ALIGN_END);
		
		gtk_box_pack_start(GTK_BOX(containerBox), temp ,1,1,1);
		gtk_widget_set_visible(temp, 1);
	}
	if( cap & V4L2_CAP_VIDEO_OUTPUT ){
		temp=gtk_label_new("Video Output(single-planar API)");
		gtk_label_set_attributes(GTK_LABEL(temp),Attrs);
		gtk_widget_set_halign (temp, GTK_ALIGN_END);
		
		gtk_box_pack_start(GTK_BOX(containerBox), temp ,1,1,1);
		gtk_widget_set_visible(temp, 1);
	}
	if( cap & V4L2_CAP_VIDEO_OVERLAY ){
		temp=gtk_label_new("Video Overlay");
		gtk_label_set_attributes(GTK_LABEL(temp),Attrs);
		gtk_widget_set_halign (temp, GTK_ALIGN_END);
		
		gtk_box_pack_start(GTK_BOX(containerBox), temp ,1,1,1);
		gtk_widget_set_visible(temp, 1);
	}
	if( cap & V4L2_CAP_VBI_CAPTURE ){
		temp=gtk_label_new("Raw VBI");
		gtk_label_set_attributes(GTK_LABEL(temp),Attrs);
		gtk_widget_set_halign (temp, GTK_ALIGN_END);
		
		gtk_box_pack_start(GTK_BOX(containerBox), temp ,1,1,1);
		gtk_widget_set_visible(temp, 1);
	}

	if( cap & V4L2_CAP_SLICED_VBI_CAPTURE ){
		temp=gtk_label_new("Sliced VBI Capture");
		gtk_label_set_attributes(GTK_LABEL(temp),Attrs);
		gtk_widget_set_halign (temp, GTK_ALIGN_END);
		
		gtk_box_pack_start(GTK_BOX(containerBox), temp ,1,1,1);
		gtk_widget_set_visible(temp, 1);
	}
	//------------------------
	if( cap & V4L2_CAP_TUNER ){
		temp=gtk_label_new("Has tuner(s) to demodulate a RF signal");
		gtk_label_set_attributes(GTK_LABEL(temp),Attrs);
		gtk_widget_set_halign (temp, GTK_ALIGN_END);
		
		gtk_box_pack_start(GTK_BOX(containerBox), temp ,1,1,1);
		gtk_widget_set_visible(temp, 1);
	}
	if( cap & V4L2_CAP_AUDIO ){
		temp=gtk_label_new("Audio I/O");
		gtk_label_set_attributes(GTK_LABEL(temp),Attrs);
		gtk_widget_set_halign (temp, GTK_ALIGN_END);
		
		gtk_box_pack_start(GTK_BOX(containerBox), temp ,1,1,1);
		gtk_widget_set_visible(temp, 1);
	}
	if( cap & V4L2_CAP_RADIO ){
		temp=gtk_label_new("This is a radio recevier");
		gtk_label_set_attributes(GTK_LABEL(temp),Attrs);
		gtk_widget_set_halign (temp, GTK_ALIGN_END);
		
		gtk_box_pack_start(GTK_BOX(containerBox), temp ,1,1,1);
		gtk_widget_set_visible(temp, 1);
	}
	if( cap & V4L2_CAP_MODULATOR ){
		temp=gtk_label_new("Has modulator(s) to emit RF signals");
		gtk_label_set_attributes(GTK_LABEL(temp),Attrs);
		gtk_widget_set_halign (temp, GTK_ALIGN_END);
		
		gtk_box_pack_start(GTK_BOX(containerBox), temp ,1,1,1);
		gtk_widget_set_visible(temp, 1);
	}
	if( cap & V4L2_CAP_SDR_CAPTURE ){
		temp=gtk_label_new("Software Defined Radio(SDR)");
		gtk_label_set_attributes(GTK_LABEL(temp),Attrs);
		gtk_widget_set_halign (temp, GTK_ALIGN_END);
		
		gtk_box_pack_start(GTK_BOX(containerBox), temp ,1,1,1);
		gtk_widget_set_visible(temp, 1);
	}
	if( cap & V4L2_CAP_EXT_PIX_FORMAT ){
		temp=gtk_label_new("Extended Pixel Format");
		gtk_label_set_attributes(GTK_LABEL(temp),Attrs);
		gtk_widget_set_halign (temp, GTK_ALIGN_END);
		
		gtk_box_pack_start(GTK_BOX(containerBox), temp ,1,1,1);
		gtk_widget_set_visible(temp, 1);
	}
	if( ((cap>>16) & 0x80)==0x80){
		temp=gtk_label_new("Metadata Capture");
		gtk_label_set_attributes(GTK_LABEL(temp),Attrs);
		gtk_widget_set_halign (temp, GTK_ALIGN_END);
		
		gtk_box_pack_start(GTK_BOX(containerBox), temp ,1,1,1);
		gtk_widget_set_visible(temp, 1);
	}
	//---------------------------------
	if( cap & V4L2_CAP_READWRITE ){
		temp=gtk_label_new(" Read/Write I/O");
		gtk_label_set_attributes(GTK_LABEL(temp),Attrs);
		gtk_widget_set_halign (temp, GTK_ALIGN_END);
		
		gtk_box_pack_start(GTK_BOX(containerBox), temp ,1,1,1);
		gtk_widget_set_visible(temp, 1);
	}
	if( cap & V4L2_CAP_ASYNCIO ){
		temp=gtk_label_new("Asynchronous I/O");
		gtk_label_set_attributes(GTK_LABEL(temp),Attrs);
		gtk_widget_set_halign (temp, GTK_ALIGN_END);
		
		gtk_box_pack_start(GTK_BOX(containerBox), temp ,1,1,1);
		gtk_widget_set_visible(temp, 1);
	}
	if( cap & V4L2_CAP_STREAMING){
		temp=gtk_label_new("Streaming I/O");
		gtk_label_set_attributes(GTK_LABEL(temp),Attrs);
		gtk_widget_set_halign (temp, GTK_ALIGN_END);
		
		gtk_box_pack_start(GTK_BOX(containerBox), temp ,1,1,1);
		gtk_widget_set_visible(temp, 1);
	}
	if( cap & V4L2_CAP_TOUCH ){
		temp=gtk_label_new("Device is a Touch Device");
		gtk_label_set_attributes(GTK_LABEL(temp),Attrs);
		gtk_widget_set_halign (temp, GTK_ALIGN_END);
		
		gtk_box_pack_start(GTK_BOX(containerBox), temp ,1,1,1);
		gtk_widget_set_visible(temp, 1);
	}
	if( cap & V4L2_CAP_DEVICE_CAPS ){
		
		PangoAttrList* Attrs2=pango_attr_list_copy(Attrs);
		pango_attr_list_insert(Attrs2, pango_attr_foreground_new(0,55535,50706));
		
		//set capability labels for just opened devie
		temp=gtk_label_new("Individual Device Specs");
		gtk_label_set_attributes(GTK_LABEL(temp),Attrs2);
		gtk_widget_set_halign (temp, GTK_ALIGN_END);
		gtk_box_pack_start(GTK_BOX(containerBox), temp ,1,1,1);
		gtk_widget_set_visible(temp, 1);
		
		//add seperator before new texts
		GtkWidget *separator=gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
		gtk_widget_show(separator);
		gtk_box_pack_start(GTK_BOX(containerBox), separator, 0,0,1);
		
		//set capability hex label
		char capability_[64];
		sprintf(capability_,"Capabilities:\t\t\t\t\t%08x",caps.device_caps);
		
		GtkWidget *temp2=gtk_label_new(capability_);
		gtk_label_set_attributes(GTK_LABEL(temp2),baseAttrs);
		gtk_widget_set_halign (temp, GTK_ALIGN_START);
		gtk_box_pack_start(GTK_BOX(containerBox), temp2 ,1,1,1);
		gtk_widget_set_visible(temp2, 1);
		//delete variables from mem
		pango_attr_list_unref(Attrs2);
		
		parse_caps(caps.device_caps, GTK_BOX(info__caps_listBox));
	}
	//delete variables from mem
	pango_attr_list_unref(Attrs);
	pango_attr_list_unref(baseAttrs);
	
}
int halocam__info_labels__activate(GtkBuilder		*builder){

	info__driver_label=GTK_WIDGET(gtk_builder_get_object(builder,"info__driver_label"));
	info__card_label=GTK_WIDGET(gtk_builder_get_object(builder,"info__card_label"));
	info__bus_label=GTK_WIDGET(gtk_builder_get_object(builder,"info__bus_label"));
	info__version_label=GTK_WIDGET(gtk_builder_get_object(builder,"info__version_label"));
	info__caps_label=GTK_WIDGET(gtk_builder_get_object(builder,"info__caps_label"));
	info__device_path=GTK_WIDGET(gtk_builder_get_object(builder,"info__device_path"));
	
	info__buff_width=GTK_WIDGET(gtk_builder_get_object(builder,"info__buff_width"));
	info__buff_height=GTK_WIDGET(gtk_builder_get_object(builder,"info__buff_height"));
	info__buff_format=GTK_WIDGET(gtk_builder_get_object(builder,"info__buff_format"));
	info__buff_colorspace=GTK_WIDGET(gtk_builder_get_object(builder,"info__buff_colorspace"));
	
	info__caps_listBox=GTK_WIDGET(gtk_builder_get_object(builder,"info__caps_listBox"));
	info__caps_extra_field=GTK_WIDGET(gtk_builder_get_object(builder,"info__caps_extra_field"));
	/*
	*
	*/
	char temp[512]={'0'};
	gtk_label_set_text(GTK_LABEL(info__device_path), Camera.name);
	
	sprintf(temp, "%s", caps.driver);
	gtk_label_set_text(GTK_LABEL(info__driver_label), temp);
	
	sprintf(temp, "%s", caps.card);
	gtk_label_set_text(GTK_LABEL(info__card_label), temp);
	
	sprintf(temp, "%s", caps.bus_info);
	gtk_label_set_text(GTK_LABEL(info__bus_label), temp);
	
	sprintf(temp, "%u.%u.%u",(caps.version>>16)&0xff, (caps.version>>8)&0xff, caps.version&0xff);
	gtk_label_set_text(GTK_LABEL(info__version_label), temp);
	
	sprintf(temp, "%08x", caps.capabilities);
	gtk_label_set_text(GTK_LABEL(info__caps_label), temp);
	//------------------------------
	sprintf(temp, "%d", fmt.fmt.pix.width);
	gtk_label_set_text(GTK_LABEL(info__buff_width), temp);
	
	sprintf(temp, "%d", fmt.fmt.pix.height);
	gtk_label_set_text(GTK_LABEL(info__buff_height), temp);
	
	sprintf(temp, "%c%c%c%c(%s)", fmt.fmt.pix.pixelformat&0xff,  (fmt.fmt.pix.pixelformat>>8)&0xff,  (fmt.fmt.pix.pixelformat>>16)&0xff,  (fmt.fmt.pix.pixelformat>>24)&0xff, fmt_desc.description);
	gtk_label_set_text(GTK_LABEL(info__buff_format), temp);
	
	//append colorspace string
	switch(fmt.fmt.pix.colorspace){
	
			case V4L2_COLORSPACE_SRGB:{
				sprintf(temp, "sRGB");
				break;
			}
			case V4L2_COLORSPACE_RAW:{
				sprintf(temp, "RAW");
				break;
			}
			case V4L2_COLORSPACE_JPEG:{
				sprintf(temp, "JPEG");
				break;
			}
			default:{
				sprintf(temp, "%u",fmt.fmt.pix.colorspace);
				break;
			}	
	}
	gtk_label_set_text(GTK_LABEL(info__buff_colorspace), temp);
	
	parse_caps(caps.capabilities, GTK_BOX(info__caps_listBox));

	return 0;
}


/*
*	CONTROLLER SIGNALS
*/
//https://stackoverflow.com/questions/60949269/executing-gtk-functions-from-other-threads
static GMutex camera_access_mutex; //shared between display and capture button
/*
*
*
*/
gboolean updateImage(GtkWidget *widget, cairo_t *cr, gpointer data){

	g_mutex_lock (&camera_access_mutex);
	
	if (imageBox!=NULL || imageBoxLayout!=NULL){
		uint8_t* buffer=(uint8_t* )camera__capture(V4L2_PIX_FMT_RGB24);//NULL is also defaults to V4L2_PIX_FMT_RGB24 
		
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
		GdkPixbuf* pixbuff_scaled=gdk_pixbuf_scale_simple (
			  pixbuff,
			  gtk_widget_get_allocated_width(imageBoxLayout),
			  gtk_widget_get_allocated_height(imageBoxLayout),
			  GDK_INTERP_BILINEAR
		);

		//3. set data to GtkImage object
		//gtk_image_set_from_pixbuf(GTK_IMAGE(imageBox), pixbuff_scaled);//set method changes imageBox ton new image  and stimulates widget
		gdk_cairo_set_source_pixbuf(cr, pixbuff_scaled, 0, 0);
		cairo_paint(cr);
		
		//last step
		g_object_unref( pixbuff_scaled );  //free pixbuff memory
		free(buffer);						   //free buffer
		
		gtk_widget_queue_draw_area(widget, 0,0, gtk_widget_get_allocated_width(imageBoxLayout),gtk_widget_get_allocated_height(imageBoxLayout));//send draw signal
		g_mutex_unlock (&camera_access_mutex);

		}
		return G_SOURCE_REMOVE;
}

gboolean captureImage(gpointer data){

	g_mutex_lock (&camera_access_mutex);
	
	char *text=data;
	text=camera__imsave(text);//get saved image name
	
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

void ShowAboutWindowButton_activate_cb(GtkMenuItem *item){
	aboutDialog__open_display_window();
}
void ShowPreferencesWindowButton_activate_cb(GtkMenuItem *item){
	cameraSettingsDialog__open_display_window();
}
gpointer app_close(gpointer GtkApp);
void app_activate (GApplication *app, gpointer user_data) {
	
	//gtk_init(&argc, &argv);//init gtk
	
	builder=gtk_builder_new_from_file("../resources/ui/halocam.glade");
	
	window=GTK_WIDGET(gtk_builder_get_object(builder,"window"));
	rootLayout=GTK_WIDGET(gtk_builder_get_object(builder,"rootLayout"));
	imageBoxLayout=GTK_WIDGET(gtk_builder_get_object(builder,"imageBoxLayout"));
	imageBox=GTK_WIDGET(gtk_builder_get_object(builder,"imageBox"));
	captureButton=GTK_WIDGET(gtk_builder_get_object(builder,"captureButton"));

	halocam__info_labels__activate(builder);//set labels for info page
	
	galleryFlowBox=GTK_WIDGET(gtk_builder_get_object(builder,"galleryFlowBox"));
	//g_signal_connect(window,"destroy",G_CALLBACK(g_application_quit),NULL);
	g_signal_connect(imageBox,"draw",G_CALLBACK(updateImage),NULL);
	gtk_builder_connect_signals(builder,NULL);
	//---
	gallery__load_all_images(galleryFlowBox, "../images/");
	gtk_window_set_application (GTK_WINDOW (window), GTK_APPLICATION (app));
	g_object_unref(builder);
	//---
	gtk_widget_show(window);
}
