fishduino
=========

An Arduino library to control the FischerTechnik Robotics Parallel interface.

I recently purchased a FischerTechnik Computing set (30554) from eBay with a second-generation parallel FischerTechnik Computing Interface (30520). This interface allowed a computer to control up to 4 motors and read up to 8 digital inputs and two analog inputs. It was connected to a computer with a long 20-pin ribbon cable and a small adapter that was different for each computer: there was one for the Apple II, one for the Commodore 64, one for any computer that has a centronics interface such as the IBM PC, etc. 

According to the web page at [1], the 30520 interface was introduced in 1991 as successor to the 30566 from 1984. Both the 30520 and the 30566 were "dumb" interfaces: they have a few shift registers, latches and timers, and the computer is expected to bitbang pins on the interface to do all the heavy work [2].

FischerTechnik released several other robot interfaces with microcontrollers afterwards, which are connected via RS-232, USB serial ports and Bluetooth serial ports. Now that operating systems such as Windows and Linux make it impossible for programs to bitbanging the pins on a printer port, that is no longer an easy option, and FischerTechnik has abandoned the parallel interfaces completely. The current software and DLL's (such as [3]) don't support the parallel interface anymore, and the older LLWin software that did support the parallel interface, doesn't run under modern versions of Windows because it was compiled as 16-bit application.

This library is an attempt to keep the parallel interface from becoming worthless because of this. Because of the way it's controlled, it's a perfect match for a microcontroller like the Arduino. 

The Fishduino library makes it possible for an Arduino application to bitbang the 30520 (and it works on the 30566 too). It's possible to write a sketch on the Arduino to read the switches and analog inputs, and control the motors, and do something useful with them. For example, you could connect the interface and a GPS module to the Arduino and build a clock that always shows the correct time.

If you want to use the 30520 interface with the current (commercial) "RoboPro" software available from FischerTechnik, you will want to download the Fishduino_Ser.ino sample sketch to your Arduino. This sketch emulates the 30420 "Intelligent Interface" which is still supported by FischerTechnik. The only caveat is that RoboPro insists on wanting this connected to one of the hardware serial ports COM1..COM4 and your Arduino probably has a USB COM port. The solution to that is to run RoboPro in a Virtual Machine that emulates the hardware COM port. I tested this with VBox and it works.

At a later time, a sketch may be implemented that will emulate the FishX1 protocol which is used by newer FischerTechnik interfaces. The protocol is partially documented in the documentation included in [3] and partially further reverse-engineered at [4]. Once that is done, the RoboPro software can be connected to the Arduino to control your hardware via the 30520 interface, and you won't have to do the detour via a VM that's necessary because RoboPro doesn't allow USB serial ports with the 30420. Functionalities such as downloading the firmware and downloading a program to run in offline mode won't be possible because the Arduino is completely different from the processor in the Robo TX interface, but controlling the parallel interface via the Arduino in "online" mode should be possible.

Jac Goudsmit
2014-06-28 Initial
2015-09-26 Updated

[1] http://www.ftcommunity.de/ftComputingFinis/interpd.htm (German)

[2] http://www.ftcommunity.de/ftComputingFinis/interfte.htm

[3] http://www.fischertechnik.de/en/ResourceImage.aspx?raid=4979

[4] http://forum.ftcommunity.de/viewtopic.php?f=8&t=1655
