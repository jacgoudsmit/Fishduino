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
  This sketch emulates the FischerTechnik 30402 "Intelligent Interface".
  See the "ft_protocol.txt" file for more information.
  
  Running this sketch should allow you to use the 30520 parallel interface
  to the RoboPro program as if it's a 30402 interface.

  The tricky part is the connection from the PC to the Arduino: RoboPro only
  allows you to connect COM1..COM4 and internally uses those strings, so
  USB COM ports (which are only available as "\\.\COMxxx", not as "COMx" --
  at least in my Windows 7 64-bit) so you really have to use a physical
  serial port on the motherboard. Bummer.

  The good news: You can use a program such as VBox in which you can run
  Windows (even a different version) and install RoboPro. Then you can set
  up a COM port in that virtual machine that redirects to a USB serial port
  on the host. I tested this and it works.
*/


/////////////////////////////////////////////////////////////////////////////
// INCLUDES
/////////////////////////////////////////////////////////////////////////////


#include <Fishduino.h>


/////////////////////////////////////////////////////////////////////////////
// TYPES
/////////////////////////////////////////////////////////////////////////////


// Type definition for state machine function
typedef void statefunc_t(byte c);


/////////////////////////////////////////////////////////////////////////////
// DATA
/////////////////////////////////////////////////////////////////////////////


Fishduino           ft;                 // Interface object
statefunc_t        *state = do_command; // Current state function pointer
unsigned            num_interfaces;     // Number of I/O bytes to expect/send
unsigned            num_received;       // Number of motor bytes so far
byte                out_data[4];        // Output bytes received
byte                want_analog;        // Requested analog input, 255=none


/////////////////////////////////////////////////////////////////////////////
// CODE
/////////////////////////////////////////////////////////////////////////////


//---------------------------------------------------------------------------
// Initialization
void setup()
{
  Serial.begin(9600);

  // Wait until serial port is ready. This is needed on some Arduinos.
  while (!Serial)
  {
    // Nothing
  }
}


//---------------------------------------------------------------------------
// Main loop
void loop()
{
  int c = Serial.read();

  if (c >= 0)
  {
    state(c);
  }
}


//---------------------------------------------------------------------------
// State function to interpret a command byte
void do_command(byte c)
{
  if (c == 0xD2)
  {
    // Old LLWin identification command
    // Just send the character back and wait for the next command
    Serial.write(c);
  }
  else if ((c >= 0xC1) && (c < 0xCD))
  {
    // Calculate number of interfaces
    num_interfaces = (((unsigned)c - 1) & 3) + 1;

    // Check if an analog input was requested
    if (c < 0xC5)
    {
      want_analog = 255;
    }
    else if (c < 0xC9)
    {
      want_analog = 0;
    }
    else
    {
      want_analog = 1;
    }

    // Initialize count of output bytes received
    num_received = 0;

    // There is always at least one output byte so set the state to the
    // output byte handler unconditionally
    state = do_output;
  }
  else
  {
    // Unknown command; just wait for the next comand
  }
}


//---------------------------------------------------------------------------
// State function to interpret motor bytes from the host
void do_output(byte c)
{
  // Store the byte
  out_data[num_received++] = c;

  // Once we got all motor bytes, send the reply and reset the state
  if (num_received == num_interfaces)
  {
    byte reply[6];
    unsigned num_to_send = num_interfaces;

    ft.SetOutputs(num_interfaces, out_data);
    ft.GetInputs(num_interfaces, reply);

    if (want_analog != 255)
    {
      unsigned analog = ft.GetAnalog(want_analog);

      // Store the word value in big-endian order
#if 0
      // TODO: scale to 0..1023
      reply[num_to_send++] = (byte)(analog >> 8);
      reply[num_to_send++] = (byte)(analog);
#else
      reply[num_to_send++] = 0;
      reply[num_to_send++] = 167;
#endif
    }

    // Send the reply
    Serial.write(reply, num_to_send);

    // Get us ready for the next command
    state = do_command;
  }
}
