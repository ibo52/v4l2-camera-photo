# v4l2-camera-photo
### take a photograph from laptop webcam.

- To control camera device, program simply uses linux/videodev2.h library (v4l2module), which comes as built-in on linux kernel.
  - _Environment: Ubuntu 20.04, gcc_
  - _kernel  : 5.15.0-41-generic_
  - _External libraries: libjpeg-turbo8-dev_
- Added filters: binary,grayscale,inverse.
