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
  - Note: Image buffer is assuming as 1 Dimensional.

# HaloCam - Take a photograph & apply effects
![app preview](https://github.com/ibo52/v4l2-camera-photo/blob/main/sample%20images/app-preview.png)
***
![app preview](https://github.com/ibo52/v4l2-camera-photo/blob/main/sample%20images/app-preview2.png)
