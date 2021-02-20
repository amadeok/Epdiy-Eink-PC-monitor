


# Epdiy Eink Pc Monitor 
(word in progress)
#### Description:
This repository contains the source code for a client a host application that allow a Epdiy eink display controller board to mirror the image of a monitor, allowing the eink display to be used as a pc monitor.
Video:

[![IMAGE ALT TEXT](http://img.youtube.com/vi/bzk12na2mWg/0.jpg)](http://www.youtube.com/watch?v=bzk12na2mWg "Video Title")

------------


#### Supported platforms:
At the moment only Linux, but will be ported for Windows. It has been tested on Ubuntu 20.04

------------


#### How it works:
The Python module mss is used to capture the screen and Pillow is used to convert the capture to black and white. The black and white image is piped to a C++ program that generates the eink framebuffer, compresses it with RLE compression and sends it wirelessly to the Epdiy board using TCP protocol.The ESP32 application for the board receives the compressed framebuffer, extracts it and uses the Epdiy driver to write it to the display. 

------------


#### Notes on development:
- Framerate and draw time depend on settings, on how much screen has changed and speed of wifi,  with default settings full frame draw time including the wifi transfer can be between 100ms to 160ms (10 fps to 6.25 fps). This will be improved in the future.
- Ghosting can be a problem in particular when drawing white letters on black background. Is recommended to use black letters on white background.
- It is necessary to have at least 1 real monitor plugged in to a port of the pc, and the application will mirror an area of that monitor, a dummy hdmi plug could be used to bypass this limitation.

------------
#### Dependencies:
This project requires a Epdiy controller board, to get one is necessary to order a pcb and solder the components. For more info go: https://github.com/vroland/epdiy

Required Python modules:
```bash
pip install mss
pip install pyautogui
pip install  numpy
```

#### Installation:

1) Flash the demo example on the example folder of the Epdiy repository to verify that it is working correctly.

3)  Get the Pc Monitor application repository:
```bash
cd
git clone https://github.com/amadeok/Epdiy-PC-monitor
cp -R ~/Epdiy-PC-monitor/pc_monitor ~/epdiy/examples 

```

4) The computer and board should connect to the same wifi network. Go to      *~/epdiy/examples/pc-monitor/main/*, open *main.c* , go to line 31  and insert the SSID of the wifi network you are going to use in the *WIFI_SSID* variable  and the password of the wifi in the *WIFI_PASS* variable.

5) Build  and flash the client application for the board:
```bash
cd ~/epdiy/examples/pc_monitor
idf.py build && idf.py flash -b 921600 && idf.py monitor
```

6)After the flashing finishes, the terminal will start displaying some messages, look for the message:
```bash
IP Address:  xxx.xxx.xxx.xxx
```

(where xxx.xxx.xxx.xxx is an ip address)

go to *~/epdiy/examples/pc_monitor/pc_host_app/*, open *main.cpp* and change the ip address of the variable *esp32-ip-address*  in line 22 to the ip address displayed in the terminal.

7) Build the pc-host application:
On a new terminal:
```bash
cd   ~/epdiy/examples/pc_monitor/pc_host_app/
g++  main.cpp generate_eink_framebuffer.cpp rle_compression.cpp utils.cpp -o process_capture -I include

```
8) Now the pc is ready to start the mirroring. If the board is ready to start mirroring it will display a message saying "Socket listening ".
To start the mirroring execute:
```bash
python3 screen_capture.py

```

Important: always exit the pc host application by pressing the letter **q**


#### Settings:
The following settings can be changed to experiment, or the defaults can also be used. 
- *greyscale_to_monochrome_threshold*  in *screen_capture.py* line 16             
When converting the capture from 256 greyscale shades to black and white monochrome a threshold is used to determine which shades will become plain black and which plain white. The default is 200 and it improves drawing black letters on white background. To change the value of that threshold change the value of the variable *greyscale_to_monochrome_threshold* 
- *x_offset* and *y_offset* in *screen_capture.py* line 13 and 14            
They set how many pixels away for the top left corner the screen will be captured. ie. setting both to zero will capture 1200x825 pixels from the extreme top left corner of the screen. setting them to 100 and 50 will capture 1200x825 pixels 100 pixels to the right and 50 pixels from the top left corner of the screen
- *sleep_time* in  *screen_capture.py* line 15
Sets the amount of time in miliseconds to sleep after each capture.
- *esp32_ip_address* in *../examples/pc_host_app/main.cpp*
Sets the ip address of the esp32 for the pc host application to connect to.
- *nb_times_to_write_framebuffer*  in *~/epdiy/examples/pc_monitor/main/pc_monitor.c* line 16                
Because of the way that eink displays work, writing the same framebuffer to the display has the effect of getting deeper blacks and whites. The variable *nb_times_to_write_framebuffer* defines the number of times to write the current eink framebuffer to the display. Increases draw time.
- *rmt_high_time* in *~/epdiy/examples/pc_monitor/main/pc_monitor.c *line 17         
Increases the high tick time of the CKV signal. A higher value makes blacks blacker and whites whiter. Increases draw time.

------------
#### Support for other displays
-The project has been tested using a board revision 4 and a ED097TC2 display. Check for the list of other  displays supported by the Epdiy board on https://github.com/vroland/epdiy . Those displays should work but are untested. If the display you want to try has the same resolution of the ED097TC2, that is 1200x825, it should be enough to select the correct display type in the menuconfig Epdiy section.
If the display you want to try has a different resolution, other than selecting it in the menuconfig, you should also:
1) open *~/epdiy/examples/pc_monitor/pc_host_app/main.cpp*, go to line 23 and change the values of the variables *width_resolution* and  *height_resolution* to the width and height resolution of your display.
2)  open  _~/epdiy/examples/pc_monitor/pc_host_app/screen_capture.py_, go to line 20 and change the values of the variables  *width_res*  and  *height_res*  to the width and height resolution of your display.
 ------------


#### To do list:
- Organize the code better
- Improve ghosting
- Optimize data transfer between pc and board
- Add support for quad grid configuration (four displays in grid configuration)


