# v4l2-camera-photo
### take a photograph from laptop webcam.

- Program simply uses linux/videodev2.h library (v4l2module) to access camera device, which comes as built-in on linux kernel,
- Then applies filters accorfing to user's chices.
# 
system | specs on | development|
--- | --- | --- |
  - _Environment| Ubuntu 22.04| gcc 11.3.0_
  - _kernel  | 5.19.0-45-generic_
  - _External libraries| libjpeg-turbo8-dev_
  ***
- Added filters: binary,grayscale,inverse, zero padding. Here is sample outputs for filters.
  - Note: Image buffer is assuming as 1 Dimensional.
  - binary filter ![sample binary filter] (https://github.com/ibo52/v4l2-camera-photo/blob/main/sample%20images/binary.jpg)
  - inverse filter ![sample inverse filter] (https://github.com/ibo52/v4l2-camera-photo/blob/main/sample%20images/inversed.jpg)
  - grayscale filter ![sample grayed filter] (https://github.com/ibo52/v4l2-camera-photo/blob/main/sample%20images/grayscaled.jpg)
  - zero padding ![sample zero padding] (https://github.com/ibo52/v4l2-camera-photo/blob/main/sample%20images/zero_padded.jpg)
<<<<<<< HEAD
>>>>>>> b70c6ce (zero pad implementation)
=======
>>>>>>> b70c6ceef281365833b38e38b6fa59961fb2c6dc
