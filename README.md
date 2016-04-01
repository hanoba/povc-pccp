# POV Cylinder Project

This repository belongs to the POV Cylinder project. A full description of this project can be found here: https://www.hackster.io/hanoba-diy/pov-cylinder-with-arduino-due-7016d5. 
The repository contains the source code for the "PC Control Program" for the POV Cylinder.


# PC Control Program for POV Cylinder

The PC Control Program (**pccp**) is a command line tool written in C++. It is running under Cygwin. It communicates with the Arduino via Bluetooth. It allows to control the POV Cylinder with the following simple single character commands:

- **0-7** - fill screen with color (black, red, yellow, green, cyan, blue, violet, white)
- **t** - draw triangle curve (as a test picture)
- **s** - enable or disable rotation of displayed picture
- **r** - draw single row 
- **c** - draw single column
- **y** - playback internal GIF picture stored in Flash memory
- **f** - download external GIF file from PC via BT
- **x** - playback downloaded external GIF file

It also provides an interface to a graphical user interface. Furthermore it displays the current rotation speed (in Hz and µs) and a frame counter value.
