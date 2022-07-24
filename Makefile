all:
	gcc cam.c filters.h filters.c -ljpeg -o cam
	@echo "compiled."
