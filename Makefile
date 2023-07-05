.PHONY=compile remove info
	
TARGET=info
all:info makedirs compile

CC=gcc
CFLAGS=-ljpeg -lm -g

PROGRAM_ROOT_DIR=halocam-v4l2
HEADER_FILES:=$(wildcard include/*.h)
SOURCE_FILES:=$(wildcard source/*.c)

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
	$(CC) $(HEADER_FILES) $(SOURCE_FILES) -o "$(PROGRAM_ROOT_DIR)/bin/camera" $(CFLAGS)
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
