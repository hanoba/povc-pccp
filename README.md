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

The full source code is available at https://github.com/hanoba/povc-pccp.git.