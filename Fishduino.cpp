/****************************************************************************
Copyright (c) 2015, Jac Goudsmit
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


#include <Arduino.h>

#include "Fishduino.h"


//---------------------------------------------------------------------------
// Initialize
bool                                  // Returns True=success False=failure
Fishduino::Reset(
  byte num_interfaces)
{
  bool result = true;

  SetNumInterfaces(num_interfaces);

  // Initialize pins
  for (unsigned u = 0; u < NumPins; u++)
  {
    pinMode(m_pin[u], u == DATACOUNTIN ? INPUT : OUTPUT);
  }

  // Reset all outputs
  SetOutputs(NULL);

  // Initialize the output from the interface (our input).
  // The three possible output sources (the two timers of the analog inputs
  // and the shift-out pin of the digital inputs) are ORed together and
  // inverted on the interface. The timers may be in a triggered state,
  // so we un-trigger them first. The digital input shifter may also be
  // in any state until we clock it enough times to shift data in from the
  // open (pulled-down) serial input of the last cascaded interface.
  // The analog input timers may take up to ~2.8ms to return their outputs
  // to LOW state (see the Analog() function). If we keep pulsing the
  // clock while we wait for the timers, all individual sources will
  // eventually turn LOW, which makes the NOR circuit (our input) go HIGH.
  unsigned long starttime = millis();

  // Un-trigger the analog inputs
  digitalWrite(m_pin[TRIGGERX], HIGH);
  digitalWrite(m_pin[TRIGGERY], HIGH);

  // Wait until the pin goes HIGH, meaning all outputs are LOW.
  while (!digitalRead(m_pin[DATACOUNTIN]))
  {
    digitalWrite(m_pin[CLOCK], LOW);
    digitalWrite(m_pin[CLOCK], HIGH);

    // Check for time-out. If this happens, something is wrong with the
    // interface or it is connected wrong.
    if (millis() - starttime >= 5)
    {
      result = false;
      break;
    }
  }

  return result;
}


//---------------------------------------------------------------------------
// Set the outputs of the interfaces
void Fishduino::SetOutputs(
  const byte *values)                 // 1 byte per interface (NULL=reset)
{
  digitalWrite(m_pin[LOADOUT], LOW);

  // Note: the following pointer is invalid is NULL is passed.
  // That's okay, we won't dereference it in that case anyway.
  const byte *p = values + m_num_interfaces;

  // Send enough output bits for the known number of interfaces.
  // If NULL is passed, send the maximum number of bits. This is sort of a
  // security feature so that "unclaimed" interfaces are cleared too.
  // Note, however, that if you call the function with a non-NULL parameter,
  // any unclaimed interfaces will get bogus output data.
  for (unsigned v = 0; v < (values ? m_num_interfaces : MaxInterfaces); v++)
  {
    // Output bits
    byte b = values ? *--p : 0;

    for (unsigned u = 0; u < 8; u++, b <<= 1)
    {
      digitalWrite(m_pin[CLOCK], LOW);
      digitalWrite(m_pin[DATAOUT], (b & 0x80) != 0);
      digitalWrite(m_pin[CLOCK], HIGH);
    }
  }

  digitalWrite(m_pin[LOADOUT], HIGH);
  digitalWrite(m_pin[LOADOUT], LOW);

  // At this point:
  // - CLOCK is HIGH
  // - DATA OUT is LOW
  // - LOAD OUT is LOW
}


//---------------------------------------------------------------------------
// Read the digital inputs from the interfaces
void Fishduino::GetInputs(
  byte *values)                       // One byte per interface
{
  // Switch the input chip to parallel mode and clock it to load the inputs
  digitalWrite(m_pin[LOADIN], HIGH);
  digitalWrite(m_pin[CLOCK], LOW);
  digitalWrite(m_pin[CLOCK], HIGH);
  digitalWrite(m_pin[LOADIN], LOW);

  if (values)
  {
    byte data;

    for (unsigned v = 0; v < m_num_interfaces; v++)
    {
      data = 0;

      for (unsigned u = 0; u < 8; u++)
      {
        data <<= 1;

        data |= (digitalRead(m_pin[DATACOUNTIN]) == LOW);

        digitalWrite(m_pin[CLOCK], LOW);
        digitalWrite(m_pin[CLOCK], HIGH);
      }

      values[v] = data;
    }
  }

  // At this point:
  // - CLOCK is high
  // - LOAD IN is LOW
}


//---------------------------------------------------------------------------
// Get an analog input
unsigned                              // Returns time (us), see above
Fishduino::GetAnalog(
  byte index)                         // 0=X, 1=Y
{
  byte trigger = m_pin[index ? TRIGGERY : TRIGGERX];

  digitalWrite(trigger, LOW);
  digitalWrite(trigger, HIGH);

  unsigned long n;
  unsigned long t = 0;

  for (n = micros(); digitalRead(m_pin[DATACOUNTIN]) == LOW; )
  {
    t = micros() - n;
    if (t > AnalogTimeout)
    {
      break;
    }
  }

  return t;
}


/////////////////////////////////////////////////////////////////////////////
// END
/////////////////////////////////////////////////////////////////////////////
