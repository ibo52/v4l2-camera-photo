#include<gtk/gtk.h>
#include <sys/types.h>
#include <unistd.h>//sleep
//#include <string.h>
#include "Camera.h"
#include "gallery.h"
#include "filters.h"
#include "about-dialog.h"
#include "camera_settings-dialog.h"
#include "halocam__window.h"

typedef struct __halocam_view{
	GtkWidget 		*window;
	GtkWidget 		*rootLayout;	//MAIN LAYOUT OF WİNDOW
	GtkWidget 		*captureButton;
	GtkWidget 		*imageBoxLayout;
	GtkWidget 		*imageBox;
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
}View;

static GtkBuilder	*builder;
static Camera	    *CameraDevice;//pointer of global defined camera
static View 		halocam;	  //View of main window

//---INFO PAGE WİDGETS---
static void* add_capability_label(GtkBox* containerBox, const char* label_text, PangoAttrList* Attrs){
	GtkWidget *temp;//label to append containerbox
	
	temp=gtk_label_new(label_text);
	gtk_label_set_attributes(GTK_LABEL(temp), Attrs);
	gtk_widget_set_halign (temp, GTK_ALIGN_END);
	gtk_box_pack_start(GTK_BOX(containerBox), temp ,1,1,1);
	gtk_widget_set_visible(temp, 1);
	
	return temp;
}
static void parse_caps(uint32_t cap, GtkBox* containerBox){
	//https://www.kernel.org/doc/html/v4.9/media/uapi/v4l/vidioc-querycap.html#id1
	
	PangoAttrList *baseAttrs = pango_attr_list_new();//text formatting
	pango_attr_list_insert(baseAttrs, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
	pango_attr_list_insert(baseAttrs, pango_attr_size_new(14*PANGO_SCALE));
	pango_attr_list_insert(baseAttrs, pango_attr_style_new(PANGO_STYLE_ITALIC));
	
	PangoAttrList* Attrs=pango_attr_list_copy(baseAttrs);
	pango_attr_list_insert(Attrs, pango_attr_foreground_new(0,32501,42501));
		
	if( cap & V4L2_CAP_VIDEO_CAPTURE )
		add_capability_label(containerBox, "Video Capture(single-planar API)", Attrs);
	
	if( cap & V4L2_CAP_VIDEO_OUTPUT )
		add_capability_label(containerBox, "Video Output(single-planar API)", Attrs);
	
	if( cap & V4L2_CAP_VIDEO_OVERLAY )
		add_capability_label(containerBox, "Video Overlay", Attrs);

	if( cap & V4L2_CAP_VBI_CAPTURE )
		add_capability_label(containerBox, "Raw VBI", Attrs);

	if( cap & V4L2_CAP_SLICED_VBI_CAPTURE )
		add_capability_label(containerBox, "Sliced VBI Capture", Attrs);
	
	if( cap & V4L2_CAP_TUNER )
		add_capability_label(containerBox, "Has tuner(s) to demodulate a RF signal", Attrs);
	
	if( cap & V4L2_CAP_AUDIO )
		add_capability_label(containerBox, "Audio I/O", Attrs);
	
	if( cap & V4L2_CAP_RADIO )
		add_capability_label(containerBox, "This is a radio recevier", Attrs);
	
	if( cap & V4L2_CAP_MODULATOR )
		add_capability_label(containerBox, "Has modulator(s) to emit RF signals", Attrs);
	
	if( cap & V4L2_CAP_SDR_CAPTURE )
		add_capability_label(containerBox, "Software Defined Radio(SDR)", Attrs);

	if( cap & V4L2_CAP_EXT_PIX_FORMAT )
		add_capability_label(containerBox, "Extended Pixel Format", Attrs);
	
	if( ((cap>>16) & 0x80)==0x80)
		add_capability_label(containerBox, "Metadata Capture", Attrs);

	if( cap & V4L2_CAP_READWRITE )
		add_capability_label(containerBox, " Read/Write I/O", Attrs);
	
	if( cap & V4L2_CAP_ASYNCIO )
		add_capability_label(containerBox, "Asynchronous I/O", Attrs);
	
	if( cap & V4L2_CAP_STREAMING)
		add_capability_label(containerBox, "Streaming I/O", Attrs);
	
	if( cap & V4L2_CAP_TOUCH )
		add_capability_label(containerBox, "Device is a Touch Device", Attrs);
	
	if( cap & V4L2_CAP_DEVICE_CAPS ){
		//set capability labels for devie as individual
		PangoAttrList* Attrs2=pango_attr_list_copy(Attrs);
		pango_attr_list_insert(Attrs2, pango_attr_foreground_new(0,55535,50706));
		GtkWidget* temp=add_capability_label(containerBox, "Individual Device Specs", Attrs2);
		gtk_widget_set_halign (temp, GTK_ALIGN_START);
		
		//add seperator before new texts
		GtkWidget *separator=gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
		gtk_widget_show(separator);
		gtk_box_pack_start(GTK_BOX(containerBox), separator, 0,0,1);
		
		//set capability hex label
		char capability_[64];
		sprintf(capability_,"Capabilities:\t\t\t\t\t%08x",CameraDevice->specs.caps.device_caps);

		GtkWidget* temp2=add_capability_label(containerBox, capability_, baseAttrs);
		gtk_widget_set_halign (temp2, GTK_ALIGN_START);
		
		//delete variables from mem
		pango_attr_list_unref(Attrs2);
		
		parse_caps(CameraDevice->specs.caps.device_caps, GTK_BOX(halocam.info__caps_listBox));
	}
	//delete variables from mem
	pango_attr_list_unref(Attrs);
	pango_attr_list_unref(baseAttrs);
	
}
static void set_info_labels(void){
	char temp[512]={'0'};
	gtk_label_set_text(GTK_LABEL(halocam.info__device_path), CameraDevice->name);
	
	sprintf(temp, "%s", CameraDevice->specs.caps.driver);
	gtk_label_set_text(GTK_LABEL(halocam.info__driver_label), temp);
	
	sprintf(temp, "%s", CameraDevice->specs.caps.card);
	gtk_label_set_text(GTK_LABEL(halocam.info__card_label), temp);
	
	sprintf(temp, "%s", CameraDevice->specs.caps.bus_info);
	gtk_label_set_text(GTK_LABEL(halocam.info__bus_label), temp);
	
	sprintf(temp, "%u.%u.%u",(CameraDevice->specs.caps.version>>16)&0xff, (CameraDevice->specs.caps.version>>8)&0xff, CameraDevice->specs.caps.version&0xff);
	gtk_label_set_text(GTK_LABEL(halocam.info__version_label), temp);
	
	sprintf(temp, "%08x", CameraDevice->specs.caps.capabilities);
	gtk_label_set_text(GTK_LABEL(halocam.info__caps_label), temp);
	//------------------------------
	sprintf(temp, "%d", CameraDevice->specs.fmt.fmt.pix.width);
	gtk_label_set_text(GTK_LABEL(halocam.info__buff_width), temp);
	
	sprintf(temp, "%d", CameraDevice->specs.fmt.fmt.pix.height);
	gtk_label_set_text(GTK_LABEL(halocam.info__buff_height), temp);
	
	sprintf(temp, "%c%c%c%c(%s)", CameraDevice->specs.fmt.fmt.pix.pixelformat&0xff,  (CameraDevice->specs.fmt.fmt.pix.pixelformat>>8)&0xff,  (CameraDevice->specs.fmt.fmt.pix.pixelformat>>16)&0xff,  (CameraDevice->specs.fmt.fmt.pix.pixelformat>>24)&0xff, CameraDevice->specs.fmt_desc.description);
	gtk_label_set_text(GTK_LABEL(halocam.info__buff_format), temp);
	
	//append colorspace string
	switch(CameraDevice->specs.fmt.fmt.pix.colorspace){
	
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
				sprintf(temp, "%u",CameraDevice->specs.fmt.fmt.pix.colorspace);
				break;
			}	
	}
	gtk_label_set_text(GTK_LABEL(halocam.info__buff_colorspace), temp);
}
int halocam__info_labels__set(void){

	halocam.info__driver_label=GTK_WIDGET(gtk_builder_get_object(builder,"info__driver_label"));
	halocam.info__card_label=GTK_WIDGET(gtk_builder_get_object(builder,"info__card_label"));
	halocam.info__bus_label=GTK_WIDGET(gtk_builder_get_object(builder,"info__bus_label"));
	halocam.info__version_label=GTK_WIDGET(gtk_builder_get_object(builder,"info__version_label"));
	halocam.info__caps_label=GTK_WIDGET(gtk_builder_get_object(builder,"info__caps_label"));
	halocam.info__device_path=GTK_WIDGET(gtk_builder_get_object(builder,"info__device_path"));
	
	halocam.info__buff_width=GTK_WIDGET(gtk_builder_get_object(builder,"info__buff_width"));
	halocam.info__buff_height=GTK_WIDGET(gtk_builder_get_object(builder,"info__buff_height"));
	halocam.info__buff_format=GTK_WIDGET(gtk_builder_get_object(builder,"info__buff_format"));
	halocam.info__buff_colorspace=GTK_WIDGET(gtk_builder_get_object(builder,"info__buff_colorspace"));
	
	halocam.info__caps_listBox=GTK_WIDGET(gtk_builder_get_object(builder,"info__caps_listBox"));
	halocam.info__caps_extra_field=GTK_WIDGET(gtk_builder_get_object(builder,"info__caps_extra_field"));
	/*
	*
	*/
	set_info_labels();

	parse_caps(CameraDevice->specs.caps.capabilities, GTK_BOX(halocam.info__caps_listBox));

	return 0;
}
int halocam__info_labels__reset(gboolean semi_reset){

	set_info_labels();
	
	if( !semi_reset ){
		GList *children, *iter;//remove all children from container
		
		children=gtk_container_get_children( GTK_CONTAINER(halocam.info__caps_listBox) );
		
		for(iter=children; iter!=NULL; iter=g_list_next(iter) )
			gtk_widget_destroy( GTK_WIDGET(iter->data) );
		
		g_list_free(children);
		parse_caps(CameraDevice->specs.caps.capabilities, GTK_BOX(halocam.info__caps_listBox));
		
		}
	
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
gboolean drawText(cairo_t *cr, char* text, gboolean show_error){
	cairo_set_source_rgb(cr, 0.1, 0.1, 0.1);
	
	cairo_select_font_face(cr, "Purisa", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
	cairo_set_font_size(cr, 22);
	cairo_move_to(cr, 20, 20);
	cairo_show_text(cr, text);
	
	if(show_error){
		cairo_show_text(cr, ":");
		cairo_show_text(cr, strerror(errno));
	}
	return G_SOURCE_REMOVE;
}

gboolean updateImage(GtkWidget *widget, cairo_t *cr, gpointer data){

	g_mutex_lock (&camera_access_mutex);
	
	if (halocam.imageBox!=NULL || halocam.imageBoxLayout!=NULL){
		cameraBuffer bufferStruct = camera__capture(CameraDevice, V4L2_PIX_FMT_RGB24);//NULL is also defaults to V4L2_PIX_FMT_RGB24 

		if ( !bufferStruct.address ){
			drawText(cr, "Buffer is Empty", 1);

		}else{
			//1. move data to gdkpixbuff struct
			GdkPixbuf* pixbuff=gdk_pixbuf_new_from_data (
				  (uint8_t*)bufferStruct.address,
				  GDK_COLORSPACE_RGB,
				  0,
				  8,//int bits_per_sample=
				  CameraDevice->specs.fmt.fmt.pix.width,
				  CameraDevice->specs.fmt.fmt.pix.height,
				  CameraDevice->specs.fmt.fmt.pix.width*3,
				  NULL,//GdkPixbufDestroyNotify destroy_fn=
				  NULL//gpointer destroy_fn_data
			);

			//2. scale gdkpixbuff iamge struct .ERROR-->Leads buffer overflow. Could not manage the struct
			GdkPixbuf* pixbuff_scaled=gdk_pixbuf_scale_simple (
				  pixbuff,
				  gtk_widget_get_allocated_width(halocam.imageBox),
				  gtk_widget_get_allocated_height(halocam.imageBox),
				  GDK_INTERP_BILINEAR
			);

			//3. set data to GtkImage object
			//gtk_image_set_from_pixbuf(GTK_IMAGE(imageBox), pixbuff_scaled);//set method changes imageBox ton new image  and stimulates widget
			gdk_cairo_set_source_pixbuf(cr, pixbuff_scaled, 0, 0);
			cairo_paint(cr);
			
			//last step
			g_object_unref( pixbuff_scaled );  //free pixbuff memory
			
			free(bufferStruct.address);
			
			gtk_widget_queue_draw_area(widget, 0,0, gtk_widget_get_allocated_width(halocam.imageBoxLayout),gtk_widget_get_allocated_height(halocam.imageBoxLayout));//send draw signal
			

			}
	}
	
	g_mutex_unlock (&camera_access_mutex);
	return G_SOURCE_REMOVE;
}

gboolean captureImage(gpointer data){

	g_mutex_lock (&camera_access_mutex);
	
	char *text=data;
	text=camera__imsave(CameraDevice, text);//get saved image name
	
	gallery__load_image(halocam.galleryFlowBox, text, 176, 176, 1);//adds image to galleryFlowBox
	
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
	cameraSettingsDialog__open_display_window(CameraDevice);
}
gpointer app_close(gpointer GtkApp);

void app_activate (GApplication *app, gpointer user_data) {
	
	CameraDevice=user_data;
	//gtk_init(&argc, &argv);//init gtk
	//printf("current package dir: %s\n", g_get_current_dir());
	builder=gtk_builder_new_from_file("../resources/view/halocam.glade");
	
	halocam.window=GTK_WIDGET(gtk_builder_get_object(builder,"window"));
	halocam.rootLayout=GTK_WIDGET(gtk_builder_get_object(builder,"rootLayout"));
	halocam.imageBoxLayout=GTK_WIDGET(gtk_builder_get_object(builder,"imageBoxLayout"));
	halocam.imageBox=GTK_WIDGET(gtk_builder_get_object(builder,"imageBox"));
	halocam.captureButton=GTK_WIDGET(gtk_builder_get_object(builder,"captureButton"));

	halocam__info_labels__set();//set labels for info page
	
	halocam.galleryFlowBox=GTK_WIDGET(gtk_builder_get_object(builder,"galleryFlowBox"));
	//g_signal_connect(window,"destroy",G_CALLBACK(g_application_quit),NULL);
	g_signal_connect(halocam.imageBox,"draw",G_CALLBACK(updateImage),NULL);
	gtk_builder_connect_signals(builder,NULL);
	//---
	gallery__load_all_images(halocam.galleryFlowBox, "../images/");
	gtk_window_set_application (GTK_WINDOW (halocam.window), GTK_APPLICATION (app));
	g_object_unref(builder);
	//---
	gtk_widget_show(halocam.window);
}
