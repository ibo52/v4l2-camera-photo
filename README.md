# v4l2-camera-photo
### take a photograph from laptop webcam.

- Program simply uses linux/videodev2.h library (v4l2module) to access camera device, which comes as built-in on linux kernel,
- Then applies filters according to user's chices.
***
system | specs | of development|
--- | --- | --- |
_Environment_| Ubuntu 22.04| gcc 11.3.0
_kernel_  | 5.19.0-45-generic_
_External libraries_| libjpeg-turbo8-dev| libgtk-3-dev
  ***
- Added filters:
  1. binary
  2. grayscale
  3. inverse
  4. zero padding
  5. gauss blur.
  6. edge detection with laplacian(3*3 kernel).
  - Note: Image buffer is assuming as 1 Dimensional. Here is sample outputs for filters.
# binary filter
![sample binary filter](https://github.com/ibo52/v4l2-camera-photo/blob/main/sample%20images/binary.jpg)
#inverse filter
![sample inverse filter](https://github.com/ibo52/v4l2-camera-photo/blob/main/sample%20images/inversed.jpg)
#grayscale filter
![sample grayed filter](https://github.com/ibo52/v4l2-camera-photo/blob/main/sample%20images/grayscaled.jpg)
#zero padding
![sample zero padding](https://github.com/ibo52/v4l2-camera-photo/blob/main/sample%20images/zero_padded.jpg)
#gaussian blur (with 11*11 kernel)
![sample blur](https://github.com/ibo52/v4l2-camera-photo/blob/main/sample%20images/gauss_blur.jpg)
#edge detection (laplacian with 3*3 kernel)
![sample blur](https://github.com/ibo52/v4l2-camera-photo/blob/main/sample%20images/laplace_edge.jpg)
