/****************************************************************************
Copyright (c) 2014, Jac Goudsmit
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

* Neither the name of the {organization} nor the names of its
contributors may be used to endorse or promote products derived from
this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
****************************************************************************/


/*
  This library provides code to control the FischerTechnik 
  30520 Universal Parallel Interface.
  It probably also works with the older 30566 but this wasn't tested.

  Schematic at:
  http://www.fischertechnik-fans.de/Images/pischaltplan.gif
  
  Code based on articles in the following two German PDF magazines:
  http://www.ftcommunity.de/ftpedia_ausgaben/ftpedia-2014-1.pdf
  http://www.ftcommunity.de/ftpedia_ausgaben/ftpedia-2014-2.pdf

  20-pole ribbon cable pinout
  ===========================
  Signal directions relative to host (Arduino).
  The pin numbers on the PCB in the 30520 interface are printed
  backwards (20<->1, 19<->2 etc).
  The analog inputs (EX and EY) run from the Fischertechnik side of the
  interface to the computer, and then are looped back to two 556 timers on
  the interface. Two loopback wires must be installed (between pins 5 and 7,
  and between pins 6 and 8) on the Arduino side of the grey ribbon cable to
  make the analog inputs work as expected. It may be possible to connect
  those pins directly to two analog input pins on the Arduino, but this is
  not supported by the library.

  +---------------+--------------+---------+-----+---------------------------
  | Pin number    | Pin number   | Default | Dir | Function
  | according to  | on Arduino   | Arduino |     |
  | interface PCB | side of grey | pin     |     |
  |               | interface    |         |     |
  |               | cable        |         |     |
  +---------------+--------------+---------+-----+---------------------------
  |      20       |       1      |   GND   | GND | GND
  |      19       |       2      |         |     | GND
  |      18       |       3      |    2    | IN  | DATA/COUNT IN
  |      17       |       4      |         |     | Not connected
  |      16       |       5      |         |     | Connect to pin 14/7
  |      15       |       6      |         |     | Connect to pin 13/8
  |      14       |       7      |         |     | Connect from pin 16/5
  |      13       |       8      |         |     | Connect from pin 15/6
  |      12       |       9      |    3    | OUT | TRIGGER X
  |      11       |      10      |    4    | OUT | TRIGGER Y
  |      10       |      11      |    5    | OUT | DATA OUT
  |       9       |      12      |    6    | OUT | CLOCK
  |       8       |      13      |    7    | OUT | LOAD OUT
  |       7       |      14      |    8    | OUT | LOAD IN
  |       6       |      15      |         |     | Not connected
  |       5       |      16      |         |     | Not connected
  |       4       |      17      |         |     | Not connected
  |       3       |      18      |         |     | Not connected
  |       2       |      19      |         |     | GND
  |       1       |      20      |         |     | GND
  +---------------+--------------+---------+---------------------------------
*/

#include <Arduino.h>

class Fishduino
{
protected:
  // These values are used to index the pin array
  enum 
  {
                                        // Pin name / Interface pin / 
                                        //   Default Arduino pin
    DATACOUNTIN,                        // DATA/COUNT IN  3  / 2
    TRIGGERX,                           // TRIGGER X      9  / 3
    TRIGGERY,                           // TRIGGER Y      10 / 4
    DATAOUT,                            // DATA OUT       11 / 5
    CLOCK,                              // CLOCK          12 / 6
    LOADOUT,                            // LOAD OUT       13 / 7
    LOADIN,                             // LOAD IN        14 / 8

    // Helper
    NumPins                             // Number of pins used
  };

  // Array that defines which interface pin is connected to which Arduino pin
  // See enum definition above for indexes.
  // This is initialized at construction time and not changed afterwards.
  byte m_pin[NumPins];

public:
  // Miscellaneous constants
  enum
  {
    AnalogTimeout = 3000,               // Analog timeout in microseconds
    MaxInterfaces = 4,                  // Max number of interfaces supported
  };

private:
  //-------------------------------------------------------------------------
  // Constructor helper
  void _Fishduino(
    byte pin_datacountin,
    byte pin_triggerx,
    byte pin_triggery,
    byte pin_dataout,
    byte pin_clock,
    byte pin_loadout,
    byte pin_loadin)
  {
    // Initialize Arduino pin numbers for each pin on the interface
    m_pin[DATACOUNTIN]    = pin_datacountin;
    m_pin[TRIGGERX] = pin_triggerx;
    m_pin[TRIGGERY] = pin_triggery;
    m_pin[DATAOUT]  = pin_dataout;
    m_pin[CLOCK]    = pin_clock;
    m_pin[LOADOUT]  = pin_loadout;
    m_pin[LOADIN]   = pin_loadin;

    // Reset outputs, initialize input
    Setup();
  }

public:
  //-------------------------------------------------------------------------
  // Constructor
  Fishduino(
    byte pin_datacountin,
    byte pin_triggerx,
    byte pin_triggery,
    byte pin_dataout,
    byte pin_clock,
    byte pin_loadout,
    byte pin_loadin)
  {
    _Fishduino(
      pin_datacountin,
      pin_triggerx,
      pin_triggery,
      pin_dataout,
      pin_clock,
      pin_loadout,
      pin_loadin);
  }

public:
  //-------------------------------------------------------------------------
  // Simpler constructor for when interface is on consecutive Arduino pins
  Fishduino(
    byte startpin = 2)
  {
    _Fishduino(
      startpin,
      startpin + 1,
      startpin + 2,
      startpin + 3,
      startpin + 4,
      startpin + 5,
      startpin + 6);
  }

public:
  //-------------------------------------------------------------------------
  // Initialize
  //
  // Initializes the port mode for all pins, and resets the outputs.
  // The input shift registers and timers are returned to a known state, so
  // the interface(s) is/are ready to read reliable values for digital and
  // analog inputs.
  //
  // The function returns false if the input can't be returned to a known
  // state in a reasonable time. This may indicate that no interface is
  // connected, or that something is wrong with it.
  //
  // This function is called by the constructor, but it can also be called
  // when you want to return the state of the interface to a known state.
  bool                                  // Returns True=success False=failure
  Setup();

public:
  //-------------------------------------------------------------------------
  // Set the outputs of the interfaces
  //
  // This has to be called every 500ms or so, otherwise a timer in the
  // interface turns the outputs off. A good way to guarantee this is to
  // use a timer interrupt. The least significant bit in each byte is O1.
  //
  // When motors are attached, the FischerTechnik software sets the lower
  // motor ports (O1, O3, O5, O7, i.e. bits 0, 2, 4, 6) when it shows
  // the motors to run counter-clockwise. It sets the higher motor ports
  // (O2, O4, O6, O8, i.e. bits 1, 3, 5, 7) when it shows the motors to run
  // clockwise.
  //
  // The first byte in the array is the interface that's directly connected
  // to the Arduino. Any further bytes represent the outputs on cascaded
  // interfaces on the expansion port of the first interface.
  //
  // Note: it's safe to specify more interfaces than are actually attached,
  // it will just take slightly longer to process. However if the caller
  // specifies fewer interfaces than are actually attached, the "unclaimed"
  // interfaces will get outputs turned off and on based on garbage data.
  void SetOutputs(
    unsigned num_interfaces,            // Number of interfaces
    const byte *values);                // 1 byte per interface (NULL=reset)

public:
  //-------------------------------------------------------------------------
  // Read the digital inputs from the interfaces
  //
  // The first byte in the array is the interface that's directly connected
  // to the Arduino. The lowest significant bit in each bit is input I1.
  //
  // Note: it's safe to specify more interfaces than are actually attached.
  // The non-attached interfaces will appear to have their digital inputs
  // set to 0. However if the caller specifies too few interfaces, the
  // following analog read may fail.
  void GetInputs(
    unsigned num_interfaces,            // Number of connected interfaces
    byte *values);                      // One byte per interface

public:
  //-------------------------------------------------------------------------
  // Get an analog input
  //
  // The return value is in microseconds. The R/C circuit in the interface
  // is dimensioned to generate a maximum interval of about 2.8 milliseconds
  // with a 5K potentiometer, but if no potentiometer is attached, the
  // function will return with AnalogTimeout as result.
  //
  // Reminder: Analog inputs from cascaded interfaces cannot be read.
  unsigned                              // Returns time (us), see above
  GetAnalog(
    byte index);                        // 0=X, 1=Y
};

