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
  This module represents motors on a FischerTechnik interface.

  IMPORTANT: These store a reference to a Fishduino manager, but there is no
  code to guard against orphaning this reference. Make sure you don't call
  any of the member functions after the manager is destroyed!

  NOTE: These only set the bits in an Fishduino manager, they don't update
  the actual interface outputs; you have to call the appropriate function in
  the manager to do that.
*/


#ifndef _FISHDUINOMOTOR_H_
#define _FISHDUINOMOTOR_H_


/////////////////////////////////////////////////////////////////////////////
// INCLUDES
/////////////////////////////////////////////////////////////////////////////


#include "FishduinoMgr.h"


/////////////////////////////////////////////////////////////////////////////
// MOTOR
/////////////////////////////////////////////////////////////////////////////


class FishduinoMotor
{
protected:
  FishduinoMgr   &m_mgr;                // Manager to work with
  byte            m_intindex;           // Interface index
  byte            m_ccwmask;            // Bits for counterclockwise
  byte            m_cwmask;             // Bits for clockwise

public:
  //-------------------------------------------------------------------------
  // Constants for better readability
  //
  // Use one of these as counterclockwise pin and use the following pin as
  // clockwise pin to connect a motor to the corresponding interface pins
  enum {
    M1 = 0,
    M2 = 2,
    M3 = 4,
    M4 = 6
  };

  enum Direction {
    CCW = -1,
    STOP = 0,
    CW = 1
  };

public:
  //-------------------------------------------------------------------------
  // Constructor
  //
  // Note: the counter-clockwise pin is usually the lower pin (even pin
  // numbers in our numbering scheme) and the clockwise pin is usually one
  // pin higher.
  FishduinoMotor(
    FishduinoMgr &mgr,                  // Manager to work with
    byte intindex,                      // Interface index
    byte ccwpin,                        // Pin to set for counter-clockwise
    byte cwpin = 255)                   // Pin to set for clockwise(255=next)
    : m_mgr(mgr)
    , m_intindex(intindex)
    , m_ccwmask(1 << ccwpin)
    , m_cwmask(1 << (cwpin == 255 ? ccwpin + 1 : cwpin))
  {
    // Nothing to do here
  }

public:
  //-------------------------------------------------------------------------
  // Stop the motor
  //
  // Note: this doesn't actually update the outputs, you need to call the
  // appropriate update function on the manager for that.
  void
    Stop()
  {
    m_mgr.SetOutputMask(m_intindex, 0, m_ccwmask | m_cwmask);
  }

public:
  //-------------------------------------------------------------------------
  // Start the motor clockwise
  //
  // Note: this doesn't actually update the outputs, you need to call the
  // appropriate update function on the manager for that.
  void
  Clockwise()
  {
    m_mgr.SetOutputMask(m_intindex, m_cwmask, m_ccwmask);
  }

public:
  //-------------------------------------------------------------------------
  // Start the motor counter-clockwise
  //
  // Note: this doesn't actually update the outputs, you need to call the
  // appropriate update function on the manager for that.
  void
  CounterClockwise()
  {
    m_mgr.SetOutputMask(m_intindex, m_ccwmask, m_cwmask);
  }

public:
  //-------------------------------------------------------------------------
  // Start the motor in the given direction
  //
  // Note: this doesn't actually update the outputs, you need to call the
  // appropriate update function on the manager for that.
  void
  Rotate(
    Direction dir)
  {
    byte m;

    if (dir == CCW)
    {
      m = m_ccwmask;
    }
    else if (dir == CW)
    {
      m = m_cwmask;
    }
    else
    {
      m = 0;
    }

    m_mgr.SetOutputMask(m_intindex, m, (m_ccwmask | m_cwmask) ^ m);
  }

public:
  //-------------------------------------------------------------------------
  // Get current state
  Direction
  GetState()
  {
    Direction result = STOP;
    byte m = (m_mgr.GetOutputMask(m_intindex) & (m_ccwmask | m_cwmask));

    if (m == m_ccwmask)
    {
      result = CCW;
    }
    else if (m == m_cwmask)
    {
      result = CW;
    }
    else
    {
      result = STOP;
    }

    return result;
  }
};


/////////////////////////////////////////////////////////////////////////////
// END
/////////////////////////////////////////////////////////////////////////////


#endif
