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
  This module represents output pins on a FischerTechnik interface.

  IMPORTANT: These store a reference to a Fishduino manager, but there is no
  code to guard against orphaning this reference. Make sure you don't call
  any of the member functions after the manager is destroyed!

  NOTE: These only set the bits in an Fishduino manager, they don't update
  the actual interface outputs; you have to call the appropriate function in
  the manager to do that.
*/


#ifndef _FISHDUINOOUTPIN_H_
#define _FISHDUINOOUTPIN_H_


/////////////////////////////////////////////////////////////////////////////
// INCLUDES
/////////////////////////////////////////////////////////////////////////////


#include "FishduinoMgr.h"


/////////////////////////////////////////////////////////////////////////////
// OUTPUT PINS
/////////////////////////////////////////////////////////////////////////////


//---------------------------------------------------------------------------
// Output Pin(s)
//
// An instance of this type can be used to set and reset multiple output
// pins at once.
class FishduinoOutPin
{
protected:
  FishduinoMgr   &m_mgr;                // Manager to work with
  byte            m_intindex;           // Interface index
  byte            m_mask;               // Bits to set/reset

public:
  //-------------------------------------------------------------------------
  // Constants for better readability
  //
  // Use these as pin number to connect a device to the corresponding output
  // pin.
  enum {
    O1 = 0,
    O2,
    O3,
    O4,
    O5,
    O6,
    O7,
    O8,

    // For those who don't like to use the letter O as identifier:...
    Q1 = 0,
    Q2,
    Q3,
    Q4,
    Q5,
    Q6,
    Q7,
    Q8,
  };

public:
  //-------------------------------------------------------------------------
  // Constructor based on single bit
  FishduinoOutPin(
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
  // Set these bits on the interface
  //
  // Note: this doesn't actually update the outputs, you need to call the
  // appropriate update function on the manager for that.
  void
  Set(
    bool value)                         // True=high, false=low
  {
    m_mgr.SetOutputMask(m_intindex, value ? m_mask : 0, value ? 0 : m_mask);
  }
};


/////////////////////////////////////////////////////////////////////////////
// END
/////////////////////////////////////////////////////////////////////////////


#endif
