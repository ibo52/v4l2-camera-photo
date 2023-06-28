.PHONY=compile remove info makedirs
	
TARGET=info
all:info makedirs compile
DIR=halocam-v4l2

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
	gcc cam.c filters.h filters.c -ljpeg -o "$(DIR)/bin/camera" -lm -g
	@echo "Done"
	@echo "---------------------------"

remove:
	@echo "Removing main directory.."
	rm -rf $(DIR)/
	@echo "Program directory removed."
	@echo "----------------------"

makedirs:
	@echo "making directories.."
	@echo "$(DIR) is MAIN DIRECTORY FOR SETUP"
	mkdir -p $(DIR)/images
	mkdir -p $(DIR)/images/ppm
	mkdir -p $(DIR)/bin
	@echo "----------------------"
