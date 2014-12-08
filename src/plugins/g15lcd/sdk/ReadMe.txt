Logitech LCD SDK for PC
Copyright (C) 2009 - 2011 Logitech Inc. All Rights Reserved


Introduction
--------------------------------------------------------------------------

This package is aimed at PC game developers and other users who are
interested in creating applications to run on a Logitech monochrome or
color LCD, such as the ones found on the G15, G19 and G510 keyboards, 
the G13, or the Z-10 speakers.
It greatly simplifies the task for creating screens by generating
bitmaps for you based on what text, progress bars or bitmaps you want
to display. Furthermore it takes care of all the plumbing that is
otherwise needed when using the Core SDK's library directly.

This package contains the following components:
- Doc folder: contains documentation that describes the api and its
  methods.
- Libs: lglcd.lib library used to connect to the LCD Manager software
  running on your machine. The LCD Manager is started automatically
  after the Logitech GamePanel Software is installed.
- Samples: sample program that demonstrates the use of every method
  available in the LCD SDK.
- Src: full source code for the LCD SDK.


Environment
--------------------------------------------------------------------------

1. Visual Studio 2005 to build and run demo samples.
2. Logitech GamePanel Software 3.06 or later installed.
3. A compatible Logitech product (G19, G510, G15, G13, Z10).


Disclaimer
--------------------------------------------------------------------------

This is work in progress. If you find anything wrong with either
documentation or code, please let us know so we can improve on it.


Where to start
--------------------------------------------------------------------------

1. Install Logitech GamePanel Software 3.06 or later.
2. Plug in a compatible Logitech device.
3. Load, compile and run the sample contained in "Samples".


For implementing LCD support in your game/applet
--------------------------------------------------------------------------
1. Add all files contained in "Src" to your project.
2. Add lgLcd.lib contained in "Lib" to your project.
3. See sample program for an example on how to use all of the
   wrapper's methods.


Where to find documentation
--------------------------------------------------------------------------

In "Doc" directory: api.html contains the necessary information for
all the available methods.


For questions, problems or suggestions email:
cj@wingmanteam.com
roland@wingmanteam.com
nanda@wingmantem.com

