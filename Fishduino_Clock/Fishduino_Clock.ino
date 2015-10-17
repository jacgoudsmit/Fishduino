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
  This project runs the FischerTechnik clock
*/


/////////////////////////////////////////////////////////////////////////////
// INCLUDES
/////////////////////////////////////////////////////////////////////////////


#include <FishduinoMotor.h>
#include <FishduinoInPin.h>


/////////////////////////////////////////////////////////////////////////////
// TYPES
/////////////////////////////////////////////////////////////////////////////


// FischerTechnik Clock hardware
FishduinoMgr        fishduino;
FishduinoMotor      motor_min( fishduino, 0, FishduinoMotor::M1);
FishduinoMotor      motor_adj( fishduino, 0, FishduinoMotor::M2);
FishduinoInPin      sensor_m(  fishduino, 0, FishduinoInPin::I1); // Cycles once/minute
FishduinoInPin      sensor_h(  fishduino, 0, FishduinoInPin::I2); // Cycles once/hour
FishduinoInPin      sensor_h12(fishduino, 0, FishduinoInPin::I3); // Cycles once/12 hours

// Actual time (that we're supposed to display)
byte                actual_hour;        // Hours mod 12 (i.e. 0..11)
byte                actual_min;         // Minutes

// Current time on the clock
byte                clock_hour = 255;   // Hours mod 12; unknown=255
byte                clock_min  = 255;   // Minutes; unknown=255


/////////////////////////////////////////////////////////////////////////////
// CODE
/////////////////////////////////////////////////////////////////////////////


void setup()
{
  Serial.begin(115200);
  Serial.println("Hello!");
}


void loop()
{
  fishduino.Update();

  // If the minute sensor turned off, turn the minute motor off
  if (sensor_m.WasOn() && sensor_m.IsOff())
  {
    motor_min.Stop();
  }


  static unsigned long m = millis() - 60001;
  unsigned long c = millis();

  if (c - m > 60000)
  {
    motor_min.Clockwise();

    m = c;
  }
}
