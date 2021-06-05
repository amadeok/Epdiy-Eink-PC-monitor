# Changelog

## [0.06] -  05-06-2021
- added draw white pixels first option
- added selective inversion option
- added smart invert option
- reduced cpu usage when screen is not updating
- added option to run download and draw on seperate task on the board

## [0.05]
- added 8 dither modes

## [0.0421] - 25-04-2021
-added option to do full refresh with epd_clear() instead of a quick one

## [0.042] - 09-04-2021
-Added option to invert image
-Added feature to choose wether hotkeys presses affect all instances of the running applications or only the one on which they are used  
-Fixed detecting mouse movement not working properly


## [0.041] - 26-03-2021
-Fixed enable_skipping option not working

## [0.04] - 25-03-2021
-Added option to apply Pillow color, contrast, brightness and sharpness enhancements to the image before it is converted to 1bpp

## [0.03] - 21-03-2021


- Added support for pseudo greyscale mode
- Added feature to choose minimum eficiency for the rle compression

## [0.02] - 19-03-2021-

- Support for using multiple displays at the same time
- Added feature to refresh screen after a certain number of frame draws
- Fixed heap corruption on ESP32 application 

## [0.01] 

- Initial release


