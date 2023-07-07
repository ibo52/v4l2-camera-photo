.PHONY=compile remove info
	
TARGET=info
all:info makedirs compile

CC=gcc
CFLAGS=-ljpeg -lm -Wall -g

PROGRAM_ROOT_DIR=halocam-v4l2

HEADER_FILES:=include/
SOURCE_FILES:=$(shell find -type f -name "*.c")

info:
	@echo "----------------------"
	@echo "Halil Ibrahim MUT"
	@echo "----------------------"
	@echo " -*-Simple camera-*-"
	@echo " take a photograph"
	@echo "  & apply effects"
	@echo "----------------------"

compile:
	@echo "compiling.."
	$(CC) $(SOURCE_FILES) -I$(HEADER_FILES) -o "$(PROGRAM_ROOT_DIR)/bin/camera" $(CFLAGS)
	@echo "Done"
	@echo "---------------------------"

remove:
	@echo "Removing main directory.."
	rm -rf $(PROGRAM_ROOT_DIR)/
	@echo "Program directory removed."
	@echo "----------------------"

makedirs:
	@echo "making directories.."
	@echo "$(PROGRAM_ROOT_DIR) is MAIN DIRECTORY FOR SETUP"
	mkdir -p $(PROGRAM_ROOT_DIR)/images
	mkdir -p $(PROGRAM_ROOT_DIR)/images/ppm
	mkdir -p $(PROGRAM_ROOT_DIR)/bin
	@echo "----------------------"
