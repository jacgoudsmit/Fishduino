fishduino
=========

An Arduino library to control the FischerTechnik Robotics Parallel interface.

I recently purchased a FischerTechnik Computing set (30554) from eBay with a parallel FischerTechnik Computing Interface (30520). This interface allowed a computer to control up to 4 motors and read up to 8 digital inputs and two analog inputs. it connected to a computer with a long 20-pin ribbon cable and a small adapter that was different for each computer: there was one for the Apple II, one for the Commodore 64, one for any computer that has a centronics interface such as the IBM PC, etc. 

According to the web page at [1], the 30520 interface was introduced in 1991 as successor to the 30566 from 1984. Both the 30520 and the 30566 were "dumb" interfaces: they have a few shift registers, latches and timers, and the computer is expected to bitbang pins on the interface to do all the heavy work [2].

FischerTechnik released several other robot interfaces with microcontrollers afterwards, which are connected via RS-232, USB serial ports and Bluetooth serial ports. Now that operating systems such as Windows and Linux separate programs from the hardware, bitbanging a printer port is no longer an easy option, and FischerTechnik has abandoned the parallel interfaces completely. The current software and DLL's (such as [3]) don't support the parallel interface anymore, and the older LLWin software that did support the parallel interface, doesn't run under modern versions of Windows because it was compiled as 16-bit application.

This library is an attempt to keep the parallel interface from becoming worthless because of this. Because of the way it's controlled, it's a perfect match for a microcontroller like the Arduino. 

The Fishduino library makes it possible for an Arduino application to bitbang the 30520 (and may work on the 30566 too): It's possible to write a sketch on the Arduino to read the switches and analog inputs, and control the motors, and do something useful with them.

At a later time, a sketch may be implemented that will emulate the FishX1 protocol (which is partially documented in the documentation included in [3] and partially further reverse-engineered at [4]), so that the current FischerTechnik software can be connected to the Arduino to control your hardware via the 30520 interface. Functionalities such as downloading the firmware and downloading a program to run in offline mode won't be possible because the Arduino is completely different from the ARM processor in the Robo TX interface, but controlling the parallel interface via the Arduino in "online" mode should be possible.

If you have a 30566 parallel interface instead of a 30520 interface, and you are willing to help me test this software with an arduino connected to that interface, please contact me.

Jac Goudsmit
2014-06-28

[1] http://www.ftcommunity.de/ftComputingFinis/interpd.htm (German)
[2] http://www.ftcommunity.de/ftComputingFinis/interfte.htm
[3] http://www.fischertechnik.de/en/ResourceImage.aspx?raid=4979
[4] http://forum.ftcommunity.de/viewtopic.php?f=8&t=1655
