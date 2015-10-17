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


/*
  This module is an extension of the FishDuino module. It keeps track of the
  inputs and outputs so that the main program doesn't have to do that. For
  example, if one function of the main program controls a motor and another
  function needs the input from one of the digital pins, neither of those
  functions have to keep track of the entire interface state to do their
  work.

  The compromise is that whenever a function reads or writes an input or
  output pin, there is a delay in getting the actual digital signals in and
  out. The effects of this problem can be minimized by calling the update
  function often enough, e.g. from your loop() function or even from a timer
  interrupt. When using a timer interrupt, it should be possible to update
  the interface so fast that PWM by bit-banging can be implemented.
*/


#ifndef _FISHDUINOMGR_H_
#define _FISHDUINOMGR_H_


/////////////////////////////////////////////////////////////////////////////
// INCLUDES
/////////////////////////////////////////////////////////////////////////////


#include "FishDuino.h"


/////////////////////////////////////////////////////////////////////////////
// FISHDUINO MANAGER
/////////////////////////////////////////////////////////////////////////////


class FishduinoMgr : public Fishduino
{
protected:
  volatile byte     m_outputs[MaxInterfaces];   // Digital outputs
  volatile byte     m_inputs[MaxInterfaces];    // Digital inputs
  byte              m_previnputs[MaxInterfaces];// Input cache

public:
  //-------------------------------------------------------------------------
  // Constructor
  FishduinoMgr(
    byte pin_datacountin,
    byte pin_triggerx,
    byte pin_triggery,
    byte pin_dataout,
    byte pin_clock,
    byte pin_loadout,
    byte pin_loadin,
    byte num_interfaces = 1)
  : Fishduino(
    pin_datacountin,
    pin_triggerx,
    pin_triggery,
    pin_dataout,
    pin_clock,
    pin_loadout,
    pin_loadin,
    num_interfaces)
  {
    Reset();
    Update();
  }

public:
  //-------------------------------------------------------------------------
  // Simpler constructor for when connected to consecutive Arduino pins
  FishduinoMgr(
    byte startpin = 2,
    byte num_interfaces = 1)
  : Fishduino(
    startpin,
    num_interfaces)
  {
    Reset();
    Update();
  }

public:
  //-------------------------------------------------------------------------
  // Reset the outputs
  void
  Reset()
  {
    memset((void*)m_outputs, 0, sizeof(m_outputs));
  }

public:
  //-------------------------------------------------------------------------
  // Update the outputs from the internal data
  void
  UpdateOutputs()
  {
    SetOutputs((const byte *)m_outputs);
  }

public:
  //-------------------------------------------------------------------------
  // Update the internal data from the inputs
  void
  UpdateInputs()
  {
    memcpy(m_previnputs, (const void *)m_inputs, sizeof(m_inputs));
    GetInputs((byte *)m_inputs);
  }

public:
  //-------------------------------------------------------------------------
  // Update the outputs and inputs
  void
  Update()
  {
    UpdateOutputs();
    UpdateInputs();
  }

public:
  //-------------------------------------------------------------------------
  // Set an output bit
  bool                                  // Returns new state of the output
  SetOutputPin(
    byte intindex,                      // Interface index, 0=first
    byte pin,                           // Output pin number, 0=first(!)
    bool value = true)                  // True=on, false=off
  {
    if ((intindex < MaxInterfaces) && (pin < 7))
    {
      if (value)
      {
        m_outputs[intindex] |= (1 << pin);
      }
      else
      {
        m_outputs[intindex] &= ~(1 << pin);
      }
    }

    return value;
  }

public:
  //-------------------------------------------------------------------------
  // Set multiple output bits
  void
  SetOutputMask(
    byte intindex,                      // Interface index, 0=first
    byte setmask,                       // Bits to set
    byte resetmask)                     // Bits to reset
  {
    m_outputs[intindex] = (m_outputs[intindex] | setmask) & (~resetmask);
  }


public:
  //-------------------------------------------------------------------------
  // Get an input bit
  bool                                  // Returns state of the given input
  GetInputPin(
    byte intindex,                      // Interface index, 0=first
    byte pin)                           // Input pin, 0=first(!)
  {
    bool result = false;

    if ((intindex < MaxInterfaces) && (pin < 7))
    {
      result = (0 != (m_inputs[intindex] & (1 << pin)));
    }

    return result;
  }

public:
  //-------------------------------------------------------------------------
  // Get previous value of input bit (from before the most recent update)
  bool                                  // Returns state of given input
  GetPrevInputPin(
    byte intindex,                      // Interface index, 0=first
    byte pin)                           // Input pin, 0=first(!)
  {
    bool result = false;

    if ((intindex < MaxInterfaces) && (pin < 7))
    {
      result = (0 != (m_previnputs[intindex] & (1 << pin)));
    }

    return result;
  }

public:
  //-------------------------------------------------------------------------
  // Get multiple input bits
  byte                                  // Returns state of entire interface
  GetInputMask(
    byte intindex)                      // Interface index, 0=first
  {
    byte result = 0;

    if (intindex < MaxInterfaces)
    {
      result = m_inputs[intindex];
    }

    return result;
  }

public:
  //-------------------------------------------------------------------------
  // Get multiple previous input bits
  byte                                  // Returns state of entire interface
    GetPrevInputMask(
    byte intindex)                      // Interface index, 0=first
  {
    byte result = 0;

    if (intindex < MaxInterfaces)
    {
      result = m_previnputs[intindex];
    }

    return result;
  }

public:
  //-------------------------------------------------------------------------
  // Get an output bit
  bool                                  // Returns state of the given output
  GetOutputPin(
    byte intindex,                      // Interface index, 0=first
    byte pin)                           // Output pin, 0=first(!)
  {
    bool result = false;

    if ((intindex < MaxInterfaces) && (pin < 7))
    {
      result = (0 != (m_outputs[intindex] & (1 << pin)));
    }

    return result;
  }

public:
  //-------------------------------------------------------------------------
  // Get all outputs of an interface
  byte                                  // Returns state as bit pattern
  GetOutputMask(
    byte intindex)                      // Interface index, 0=first
  {
    bool result = 0;

    if (intindex < MaxInterfaces)
    {
      result = m_outputs[intindex];
    }

    return result;
  }
};


/////////////////////////////////////////////////////////////////////////////
// END
/////////////////////////////////////////////////////////////////////////////


#endif
