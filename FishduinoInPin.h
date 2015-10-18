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
  This module represents input pins on a FischerTechnik interface.

  IMPORTANT: These store a reference to a Fishduino manager, but there is no
  code to guard against orphaning this reference. Make sure you don't call
  any of the member functions after the manager is destroyed!

  NOTE: These only set the bits in an Fishduino manager, they don't update
  the actual interface outputs; you have to call the appropriate function in
  the manager to do that.
*/


#ifndef _FISHDUINOINPIN_H_
#define _FISHDUINOINPIN_H_


/////////////////////////////////////////////////////////////////////////////
// INCLUDES
/////////////////////////////////////////////////////////////////////////////


#include "FishduinoMgr.h"


/////////////////////////////////////////////////////////////////////////////
// INPUT PINS
/////////////////////////////////////////////////////////////////////////////


//---------------------------------------------------------------------------
// Input Pin(s)
//
// An instance of this type can be used to get the value of one or more
// digital input pins.
class FishduinoInPin
{
protected:
  FishduinoMgr   &m_mgr;                // Manager to work with
  byte            m_intindex;           // Interface index
  byte            m_mask;               // Bits to set/reset

public:
  //-------------------------------------------------------------------------
  // Constants for better readability
  //
  // Use these as pin number to connect a device to the corresponding input
  // pin.
  enum {
    I1 = 0,
    I2,
    I3,
    I4,
    I5,
    I6,
    I7,
    I8
  };

public:
  //-------------------------------------------------------------------------
  // Constructor based on single bit
  FishduinoInPin(
    FishduinoMgr &mgr,                  // Manager to work with
    byte intindex,                      // Interface index
    byte pin)                           // Pin number (0..7)
    : m_mgr(mgr)
    , m_intindex(intindex)
    , m_mask(1 << pin)
  {
    // Nothing to do here
  }

public:
  //-------------------------------------------------------------------------
  // Add a bit to the mask
  void
  AddPin(
    byte pin)                           // Pin to add
  {
    m_mask |= 1 << pin;
  }

public:
  //-------------------------------------------------------------------------
  // Remove a bit from the mask
  void
  RemovePin(
    byte pin)                           // Pin to remove
  {
    m_mask &= ~(1 << pin);
  }

public:
  //-------------------------------------------------------------------------
  // Get these bits on the interface
  //
  // If the bool parameter is false (default), the function returns true if
  // ANY of the bits are high; if the parameter is true, it returns true only
  // if ALL relevant bits are high.
  //
  // Note: this doesn't actually update the value from the inputs, you need
  // to call the appropriate update function on the manager for that.
  bool                                  // Returns true=pin is on
  Get(
    bool all = false)                   // True=AND pins, false=OR pins
  {
    byte b = m_mgr.GetInputMask(m_intindex) & m_mask;

    return all ? (b != 0) : (b == m_mask);
  }

public:
  //-------------------------------------------------------------------------
  // Check if pins in our mask have the given state
  //
  // If both parameters are true, the function returns true if the pins in
  // the mask are either all on or all off, false if some pins are on and
  // some are off.
  //
  // If both parameters are false, the function returns true if and only if
  // some pins are on but not none and not all.
  bool
  Is(
    bool off,                           // True=return true if all pins off
    bool on)                            // True=return true if all pins on
  {
    bool result;
    byte b = m_mgr.GetInputMask(m_intindex) & m_mask;

    if (b == 0)
    {
      result = off;
    }
    else if (b == m_mask)
    {
      result = on;
    }
    else
    {
      result = ((!on) && (!off));
    }

    return result;
  }

public:
  //-------------------------------------------------------------------------
  // Check if pins in our mask are all off
  bool IsOff()
  {
    return Is(true, false);
  }

public:
  //-------------------------------------------------------------------------
  // Check if pins in our mask are all on
  bool IsOn()
  {
    return Is(false, true);
  }

public:
  //-------------------------------------------------------------------------
  // Check if pins in our mask had the given state before the last update
  //
  // If both parameters are true, the function returns true if the pins in
  // the mask were either all on or all off, false if some pins were on and
  // some were off.
  //
  // If both parameters are false, the function returns true if and only if
  // some pins were on but not none and not all.
  bool
  Was(
    bool off,                           // True=return true if all pins off
    bool on)                            // True=return true if all pins on
  {
    bool result;
    byte b = m_mgr.GetPrevInputMask(m_intindex) & m_mask;

    if (b == 0)
    {
      result = off;
    }
    else if (b == m_mask)
    {
      result = on;
    }
    else
    {
      result = ((!on) && (!off));
    }

    return result;
  }


public:
  //-------------------------------------------------------------------------
  // Check if pins in our mask were all off before the last update
  bool WasOff()
  {
    return Was(true, false);
  }

public:
  //-------------------------------------------------------------------------
  // Check if pins in our mask were all on before the last update
  bool WasOn()
  {
    return Was(false, true);
  }
};


/////////////////////////////////////////////////////////////////////////////
// END
/////////////////////////////////////////////////////////////////////////////


#endif
