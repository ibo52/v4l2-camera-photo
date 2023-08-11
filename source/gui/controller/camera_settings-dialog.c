#include<gtk/gtk.h>
#include<stdint.h>
#include <sys/ioctl.h>
#include "cam.h"

GtkBuilder		*cameraSettingsDialog__builder;
GtkWidget		*cameraSettingsDialog__display_window;
GtkWidget		*rootBox;
GtkWidget		*format_ResolutionComboBox;
GtkWidget		*format_deviceListComboBox;

#define CLEAR(x) memset(&(x), 0, sizeof(x)) //write zero to the struct space

void cameraCtrl_slider_value_changed_cb(GtkRange* self, gpointer ctrl_id){
	
	//forward control id and value to camera
	camera__control__set((uintptr_t)ctrl_id ,(int)gtk_range_get_value(self) );
}

void cameraCtrl_comboBox_value_changed_cb(GtkComboBox* self, gpointer ctrl_id){
	
	//forward control id and value to camera
	int ctrl_value = atoi( gtk_combo_box_get_active_id(self) );
	camera__control__set((uintptr_t)ctrl_id , ctrl_value );
}

void cameraCtrl_switch_value_changed_cb(GtkSwitch* self,gboolean state, gpointer ctrl_id){
	
	//forward control id and value to camera
	camera__control__set((uintptr_t)ctrl_id ,(int)state );
}

void formatCtrl_comboBox_value_changed_cb(GtkComboBox* self, gpointer ctrl_id){
	
	/*
	int width, height;
	char *string= gtk_combo_box_text_get_active_text( GTK_COMBO_BOX_TEXT(self) );
	
	char* height_str=strchr(string, 'x');
	
	char width_str[10]={'0'};
	strncpy(width_str, string, (height_str-string));
	width=atoi(width_str);
	
	*height_str++;
	height=atoi(height_str);
	
	g_print("string:%s  h:%i w:%i\n", string, height ,width);
	*/
	//set_format(width, height, 0, 0);
	
}

void format_deviceListcomboBox_value_changed_cb(GtkComboBox* self, gpointer user_data){
	
	//close the one, and open selected camera device
	camera__deactivate();
	camera__activate(  gtk_combo_box_text_get_active_text( GTK_COMBO_BOX_TEXT(self) )  );
}

static void cameraSettingsDialog__append_formatControls(){
	
	int index=0;
	CLEAR(frame_size);
	char tempString[32];
	char tempId[4];
	
	while(-1 != get_frameSize(index) ){
		
		sprintf(tempString, "%ix%i", frame_size.discrete.width, frame_size.discrete.height);
		sprintf(tempId, "%i", index);
		gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(format_ResolutionComboBox), tempId, tempString);
		index++;
	}
	gtk_combo_box_set_active(GTK_COMBO_BOX(format_ResolutionComboBox), 0);
	g_signal_connect(format_ResolutionComboBox, "changed", G_CALLBACK(formatCtrl_comboBox_value_changed_cb), NULL);
	
	/*
	*/
	GDir *Dir=g_dir_open("/dev",0,NULL);
	const char* file;
	
	while( ( file=g_dir_read_name(Dir) )!=NULL ){

		if ( strstr(file, "video")!=NULL ){
			char deviceX[32];
			strcpy(deviceX, "/dev/");
			strcat(deviceX, file);
			
			gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(format_deviceListComboBox), deviceX, deviceX);
		}
	}
	g_signal_connect(format_deviceListComboBox, "changed", G_CALLBACK(format_deviceListcomboBox_value_changed_cb), NULL);
	g_dir_close(Dir);
}

static void cameraSettingsDialog__append_controls(){
	/*
	*	Check and append all supported controls to menu
	*/
	PangoAttrList *Attrs = pango_attr_list_new();//text formatting
	pango_attr_list_insert(Attrs, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
	pango_attr_list_insert(Attrs, pango_attr_foreground_new(0,16384,32768));
	
	CLEAR(queryctrl);
	queryctrl.id = V4L2_CID_USER_CLASS;//V4L2_CID_BASE;
	
	while (0 == camera__control__get_ctrl() ) {

			if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
				continue; //if control not supported
			
			//box properties
			GtkWidget* box=gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
			gtk_box_set_homogeneous(GTK_BOX(box), 1);
			
			//label properties
			GtkWidget* label=gtk_label_new( (char *)queryctrl.name );
			gtk_label_set_attributes(GTK_LABEL(label),Attrs);
			gtk_widget_set_halign(label, GTK_ALIGN_START);			
			gtk_widget_set_margin_start(label, 5);
			
			//if reached V4L2_CID_CAMERA_CLASS or V4L2_CID_USER_CLASS do not add slider.
			//These are not controls, but explanation strings for next control segments
			
			if( ((queryctrl.id^V4L2_CTRL_FLAG_NEXT_CTRL)==V4L2_CID_CAMERA_CLASS) | //https://docs.nvidia.com/jetson/l4t-multimedia/v4l2-controls_8h.html
			((queryctrl.id^V4L2_CTRL_FLAG_NEXT_CTRL)==V4L2_CID_USER_CLASS)  ||
			((queryctrl.id^V4L2_CTRL_FLAG_NEXT_CTRL)==V4L2_CTRL_CLASS_MPEG+1) ||
			((queryctrl.id^V4L2_CTRL_FLAG_NEXT_CTRL)==V4L2_CTRL_CLASS_FLASH+1)||
			((queryctrl.id^V4L2_CTRL_FLAG_NEXT_CTRL)==V4L2_CTRL_CLASS_JPEG+1) ||
			((queryctrl.id^V4L2_CTRL_FLAG_NEXT_CTRL)==V4L2_CTRL_CLASS_IMAGE_SOURCE+1) ||
			((queryctrl.id^V4L2_CTRL_FLAG_NEXT_CTRL)==V4L2_CTRL_CLASS_IMAGE_PROC+1)   ||
			((queryctrl.id^V4L2_CTRL_FLAG_NEXT_CTRL)==V4L2_CTRL_CLASS_DV+1) 			||
			((queryctrl.id^V4L2_CTRL_FLAG_NEXT_CTRL)==V4L2_CTRL_CLASS_DETECT+1) 	    ){
										
				gtk_widget_set_halign(label, GTK_ALIGN_CENTER);
				
				gtk_box_pack_start(GTK_BOX(box), label, 1,1,5);
				gtk_box_pack_start(GTK_BOX(rootBox), box, 1,1,5);
				gtk_widget_show(label);	
				gtk_widget_show(box);
				continue;
			}
			//slider properties
			GtkWidget* slider;
			if(queryctrl.type == V4L2_CTRL_TYPE_MENU){ //if it has menu type controls
			
				slider=gtk_combo_box_text_new();
				queryctrl.id=(queryctrl.id^V4L2_CTRL_FLAG_NEXT_CTRL);//reverse effect of flag V4L2_CTRL_FLAG_NEXT_CTRL
				
				for (querymenu.index = queryctrl.minimum; querymenu.index <= queryctrl.maximum; querymenu.index++) {
         			
         			camera__control__enumerate_menu();
         			
         			char temp[20];
         			sprintf(temp, "%i",querymenu.index);
         			
         			gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(slider), temp, (char*)querymenu.name);
         			
				}
				gtk_combo_box_set_active(GTK_COMBO_BOX(slider), queryctrl.default_value);
				g_signal_connect(slider, "changed", G_CALLBACK(cameraCtrl_comboBox_value_changed_cb), (gpointer)(uintptr_t)(queryctrl.id^V4L2_CTRL_FLAG_NEXT_CTRL));
				
				queryctrl.id=(queryctrl.id|V4L2_CTRL_FLAG_NEXT_CTRL);//de-reverse effect of flag V4L2_CTRL_FLAG_NEXT_CTRL;	
			}
			else if( queryctrl.type == V4L2_CTRL_TYPE_BOOLEAN ){
				slider=gtk_switch_new();
				gtk_widget_set_halign(slider, GTK_ALIGN_START);
				gtk_widget_set_margin_end(slider, 50);
				
				g_signal_connect(slider, "state-set", G_CALLBACK(cameraCtrl_switch_value_changed_cb), (gpointer)(uintptr_t)(queryctrl.id^V4L2_CTRL_FLAG_NEXT_CTRL));
			}
			else{
			slider=gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL,
												queryctrl.minimum, queryctrl.maximum, 1);				
			gtk_range_set_value(GTK_RANGE(slider), queryctrl.default_value);
			gtk_scale_add_mark(GTK_SCALE(slider), (float)queryctrl.default_value, GTK_POS_BOTTOM, "|");
			gtk_widget_set_halign(slider, GTK_ALIGN_FILL);
			gtk_widget_set_margin_end(slider, 50);
			}
			//pack and show widgets
			gtk_box_pack_start(GTK_BOX(box), label, 1,1,0);
			gtk_box_pack_start(GTK_BOX(box), slider, 1,1,0);
			gtk_box_pack_start(GTK_BOX(rootBox), box, 1,1,5);
			
			gtk_widget_show(label);
			gtk_widget_show(slider);
			gtk_widget_show(box);
			
			if (queryctrl.flags & V4L2_CTRL_FLAG_INACTIVE)
				gtk_widget_set_sensitive(slider, 0);//disable if flag set
				
			else if( GTK_IS_SCALE(slider))
				g_signal_connect(slider, "value-changed", G_CALLBACK(cameraCtrl_slider_value_changed_cb),(gpointer)(uintptr_t)(queryctrl.id^V4L2_CTRL_FLAG_NEXT_CTRL));//XOR to get control id

	}
	pango_attr_list_unref(Attrs);
}

int cameraSettingsDialog__open_display_window(){

	if (cameraSettingsDialog__display_window!=NULL) {

		gtk_window_close (GTK_WINDOW(cameraSettingsDialog__display_window));
	}
		
		cameraSettingsDialog__builder=gtk_builder_new_from_file("../resources/ui/camera_settings.glade");
	
		cameraSettingsDialog__display_window=GTK_WIDGET(gtk_builder_get_object(cameraSettingsDialog__builder,"cameraSettingsDialog"));	
		rootBox=GTK_WIDGET(gtk_builder_get_object(cameraSettingsDialog__builder,"rootBox"));	
		format_ResolutionComboBox=GTK_WIDGET(gtk_builder_get_object(cameraSettingsDialog__builder,"format_ResolutionComboBox"));
		format_deviceListComboBox=GTK_WIDGET(gtk_builder_get_object(cameraSettingsDialog__builder,"format_deviceListComboBox"));
		
		cameraSettingsDialog__append_controls();
		cameraSettingsDialog__append_formatControls();
		
		g_signal_connect(cameraSettingsDialog__display_window,"destroy",G_CALLBACK(gtk_window_close),NULL);
		
		gtk_widget_show_all (cameraSettingsDialog__display_window);
		
	return 0;
}
