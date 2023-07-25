.PHONY=all remove info
	
TARGET=info
all:info makedirs copyResources compile

CC=gcc
CLIBS=`pkg-config --libs gtk+-3.0` -ljpeg -lm
CFLAGS= -Wall -g -rdynamic `pkg-config --cflags gtk+-3.0`

PROGRAM_ROOT_DIR=halocam-v4l2

HEADER_FILES:=include/
SOURCE_FILES:=$(shell find -type f -name "*.c")


make_std_color=\033[3$1m

RESET_COLOR=\033[0m
ERROR_COLOR = $(strip $(call make_std_color,1))
INFO_COLOR = $(strip $(call make_std_color,3))
CYAN_COLOR=  $(strip $(call make_std_color,6))

info:
	@echo "\t\t$(INFO_COLOR)======================"
	@echo "\t\t  Halil Ibrahim MUT"
	@echo "\t\t----------------------"
	@echo "\t\t -*-$(CYAN_COLOR)Simple camera$(INFO_COLOR)-*-"
	@echo "\t\t $(CYAN_COLOR)take a photograph"
	@echo "\t\t  & apply effects$(INFO_COLOR)"
	@echo "\t\t======================$(RESET_COLOR)\n"

compile:
	@echo "$(INFO_COLOR)compiling.."
	@echo "======================$(RESET_COLOR)"
	$(CC) $(CFLAGS) $(SOURCE_FILES) -I$(HEADER_FILES) -o "$(PROGRAM_ROOT_DIR)/bin/camera" $(CLIBS)
	@echo "\n"
	
	@echo "Done"

remove:
	@echo "$(INFO_COLOR)Removing main Directory.."
	@echo "======================$(RESET_COLOR)"
	rm -rf $(PROGRAM_ROOT_DIR)/
	@echo "\n"
	@echo "Program directory removed."

makedirs:
	@echo "$(INFO_COLOR)making directories.."
	@echo "======================$(RESET_COLOR)"
	@echo "$(PROGRAM_ROOT_DIR) is MAIN DIRECTORY FOR SETUP"
	mkdir -p $(PROGRAM_ROOT_DIR)/images
	mkdir -p $(PROGRAM_ROOT_DIR)/images/ppm
	mkdir -p $(PROGRAM_ROOT_DIR)/bin
	@echo "\n"
	
copyResources:
	@echo "$(INFO_COLOR)Copying resource files"
	@echo "======================$(RESET_COLOR)"
	cp source/gui/halocam.glade $(PROGRAM_ROOT_DIR)/bin/halocam.glade
	cp source/gui/gallery.glade $(PROGRAM_ROOT_DIR)/bin/gallery.glade
	@echo "\n"
