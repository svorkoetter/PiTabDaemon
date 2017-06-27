PiTabDaemon is a background process for monitoring and controlling the state of
a Raspberry Pi based tablet, as described here (link coming soon). It performs
the following functions, once per millisecond unless otherwise noted:

* checks for a shutdown signal from the power switch and initiates a proper shutdown.
* monitors the three buttons and performs the corresponding actions:

** bring keyboard (short press) or dashboard (long press) to front
** increase brightness by 1/8 (short press) or to maximum (long press)
** toggle foreground application between normal and maximized (short press) or full screen (long press)

* monitors commands from the dashboard (every 5 seconds):

** enable/disable screen dimming on idle when on battery power
** enable/disable USB and Ethernet ports
** enable/disable Wi-Fi and Bluetooth

* power monitoring:

** status of PowerBoost 1000C charging and charge-completed indicators
** battery voltage
** estimate of energy remaining
** information is written to a tiny RAM disk for display by dashboard

* monitors X11 idle time (at varying intervals depending on need):

** dims display to half of selected brightness after 2 minutes of inactivity
** turns off backlight completely after 5 minutes

* monitors PowerBoost 1000C LBO and performs an immediate shutdown if triggered.

The daemon makes use of the following open source libraries and utilities:

* bcm2835 - low level GPIO library used to monitor buttons, voltage, etc.
* libxss and xscreensaver - allows the daemon to monitor user idle time.
* wmctrl - command line utility used by the daemon to resize windows.
