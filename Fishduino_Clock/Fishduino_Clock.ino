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
FishduinoInPin      sensor_h(  fishduino, 0, FishduinoInPin::I3); // Cycles once/hour
FishduinoInPin      sensor_h12(fishduino, 0, FishduinoInPin::I2); // Cycles once/12 hours

// Actual time (that we're supposed to display)
byte                actual_hour;        // Hours mod 12 (i.e. 0..11)
byte                actual_min;         // Minutes

// Current time on the clock
byte                clock_hour = 255;   // Hours mod 12; unknown=255
byte                clock_min  = 255;   // Minutes; unknown=255


/////////////////////////////////////////////////////////////////////////////
// CODE
/////////////////////////////////////////////////////////////////////////////


//---------------------------------------------------------------------------
// Logic to control a motor with a sensor
//
// Runs the motor in the given direction whenever, and as long as the given
// sensor is on. When the sensor turns off, the motor is stopped, but once
// the motor is stopped, its direction isn't changed.
//
// This can be called repeatedly (with calls to the Fishduino::Update 
// function in between) to implement an axis (DOF) that returns to its
// original position once it gets started, and only requires one start
// command.
void
RotateUntilSensor(
  FishduinoMotor &motor,                // Motor to use
  FishduinoMotor::Direction dir,        // Direction to rotate to return home
  FishduinoInPin &sensor,               // Input pin to check
  bool off,                             // true=run until all pins off
  bool on)                              // true=run until all pins on
{
  if (sensor.Is(off, on))
  {
    // Stop the motor if it just reached the desired state.
    // If it was already in the desired state, don't change the state, so
    // that a single command from elsewhere to start the motor is enough to
    // keep it running until the desired state.
    if (!sensor.Was(off, on))
    {
      motor.Stop();
    }
  }
  else
  {
    // The sensor is in another state, keep the motor running
    motor.Rotate(dir);
  }
}

void setup()
{
  Serial.begin(115200);
  Serial.println("Hello!");
}


void loop()
{
  fishduino.Update();

  RotateUntilSensor(motor_min, FishduinoMotor::CW, sensor_m, true, false);

  static unsigned long m = millis() - 60001;
  unsigned long c = millis();

  if (c - m > 60000)
  {

    motor_min.Clockwise();

    m = c;
  }
}


/////////////////////////////////////////////////////////////////////////////
// END
/////////////////////////////////////////////////////////////////////////////
