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
#include <Streaming.h>

// Include files needed for gpstime.h
#include <SoftwareSerial.h>
#include <TinyGPS.h> // http://arduiniana.org/libraries/tinygps/
#include <Time.h> // http://www.pjrc.com/teensy/td_libs_Time.html
#include "gpstime.h"


/////////////////////////////////////////////////////////////////////////////
// MACROS
/////////////////////////////////////////////////////////////////////////////


// Workaround for functions that use user types and normally can't be used
// in sketches because the Arduino compiler tries to generate a prototype
// which won't work without the typedef.
#define SILLY_ARDUINO(x) x

// Common trick to determine the length of an array
#define ARR_LEN(x) (sizeof(x) / sizeof(x[0]))


/////////////////////////////////////////////////////////////////////////////
// TYPEDEF
/////////////////////////////////////////////////////////////////////////////


// State machine events
typedef enum
{
  EventNone,                            // Nothing to do (used internally)
  EventInit,                            // Entering state func for 1st time
  EventMinuteUp,                        // Minute sensor forward
  EventMinuteDown,                      // Minute sensor backward
  EventHourUp,                          // Hour sensor forward
  EventHourDown,                        // Hour sensor backward
  EventTime,                            // The actual time changed

  EventNum
} Event;

// State machine states
typedef enum
{
  StateIdle,                            // No motors running
  StateCalibrating,                     // Figuring out where the hands are
  StateAdvancingMinute,                 // Handling elapsed time
  StateAdjusting,                       // Adjusting, e.g. time zone changed

  StateError,                           // Error state, something went wrong
  StateNum
} State;

// State machine event handlers are functions that use the same prototype.
#define EVENTHANDLER_PROTO(x) State x(Event event)
typedef EVENTHANDLER_PROTO(EventHandlerCB);


/////////////////////////////////////////////////////////////////////////////
// DATA
/////////////////////////////////////////////////////////////////////////////


// FischerTechnik Clock hardware
FishduinoMgr        fishduino;
FishduinoMotor      motor_min( fishduino, 0, FishduinoMotor::M1);
FishduinoMotor      motor_adj( fishduino, 0, FishduinoMotor::M2);
FishduinoInPin      sensor_m(  fishduino, 0, FishduinoInPin::I1); // Cycles once/minute
FishduinoInPin      sensor_h(  fishduino, 0, FishduinoInPin::I3); // Cycles once/hour
FishduinoInPin      sensor_h12(fishduino, 0, FishduinoInPin::I2); // Cycles once/12 hours

// Actual time (that we're supposed to display)
byte                actual_hour = 255;  // Hours mod 12 (i.e. 0..11); unknown=255
byte                actual_min = 255;   // Minutes; unknown=255

// Current time on the clock
byte                clock_hour = 255;   // Hours mod 12; unknown=255
byte                clock_min  = 255;   // Minutes; unknown=255

// Forward declarations for event handlers
EventHandlerCB
  state_Idle,
  state_Calibrating,
  state_AdvancingMinute,
  state_Adjusting,
  state_Error;

// Event handler table
// This is used to look up the event handler, based on the current state.
// There's also a string to print for debugging
const struct statestruct
{
  State             state;
  EventHandlerCB   *func;
  const char       *name;
} statetable[] =
{
#define E(x) { State##x, state_##x, #x },
  E(Idle)
  E(Calibrating)
  E(AdvancingMinute)
  E(Adjusting)
  E(Error)
#undef E
};

// GPS time keeper
GPSTime gps(10, 9);


/////////////////////////////////////////////////////////////////////////////
// ACTUAL TIME FUNCTIONS
/////////////////////////////////////////////////////////////////////////////


//---------------------------------------------------------------------------
// Calculate difference in minutes between clock time and actual time
//
// The result is positive if the clock needs to go forward.
// If the actual time or clock time is unknown, the result is 0.
int GetTimeDiff()
{
  int result = 0;

  if ((clock_hour != 255) && (clock_min != 255) && (actual_hour != 255) && (actual_min != 255))
  {
    unsigned clocktime = (unsigned)clock_hour * 60 + (unsigned)clock_min;
    unsigned actualtime = (unsigned)actual_hour * 60 + (unsigned)actual_min;

    result = (int)(actualtime - clocktime);

    // If the difference is more than 6 hours, go the other way instead
    if (result > 360)
    {
      result -= 720;
    }
    else if (result < -360)
    {
      result += 720;
    }
  }

  return result;
}


//---------------------------------------------------------------------------
// Calculate difference in hours between clock time and actual time
//
// If the actual minutes are known, the result is 0 if the clock minutes are
// within +/- 30 minutes of the actual time.
// So for example, if the clock time is 1:00, the function returns 1 for
// actual times between 1:30 and 1:29.
int GetHourDiff()
{
  int result = 0;

  if ((clock_hour != 255) && (actual_hour != 255))
  {
    result = (unsigned)actual_hour - (unsigned)clock_hour;

    if (actual_min != 255)
    {
      if (actual_min >= 30)
      {
        result++;
      }
    }

    if (clock_min != 255)
    {
      if (clock_min >= 30)
      {
        result--;
      }
    }

    // If the difference is more than 6 hours, go the other way instead
    if (result > 6)
    {
      result -= 12;
    }
    else if (result < -6)
    {
      result += 12;
    }
  }

  return result;
}


/////////////////////////////////////////////////////////////////////////////
// CLOCK TIME FUNCTIONS
/////////////////////////////////////////////////////////////////////////////


//---------------------------------------------------------------------------
// Add one hour to the clock time if we know it
void ClockHourUp()
{
  if (clock_hour != 255)
  {
    if (++clock_hour == 12)
    {
      clock_hour = 0;
    }
  }
}


//---------------------------------------------------------------------------
// Subtract one hour from the clock time if we know it
void ClockHourDown()
{
  if (clock_hour != 255)
  {
    if (clock_hour-- == 0)
    {
      clock_hour = 11;
    }
  }
}

//---------------------------------------------------------------------------
// Add one minute to the clock time if we know it
void ClockMinuteUp()
{
  if (clock_min != 255)
  {
    if (++clock_min == 60)
    {
      clock_min = 0;

      ClockHourUp();
    }
  }
}


//---------------------------------------------------------------------------
// Subtract one minute from the clock time if we know it
void ClockMinuteDown()
{
  if (clock_min != 255)
  {
    if (clock_min-- == 0)
    {
      clock_min = 59;

      ClockHourDown();
    }
  }
}


/////////////////////////////////////////////////////////////////////////////
// EVENT HANDLERS
/////////////////////////////////////////////////////////////////////////////


//---------------------------------------------------------------------------
// Event handler for idle state
//
// This is the initial state. The motors are stopped.
EVENTHANDLER_PROTO(state_Idle)
{
  State result = StateIdle;

  switch (event)
  {
  case EventInit:
    // Init the state: stop the motors.
    motor_adj.Stop();
    motor_min.Stop();
    Serial << "Idle: Init" << endl;
    break;

  case EventTime:
    // Check if the clock time is 60 minutes of less from the actual time.
    // If so, start the minute motor and go to the AdvancingMinute state.
    {
      int diff = GetTimeDiff();
      int absdiff = abs(diff);

      if (absdiff)
      {
        Serial << "Idle: Time changed by " << diff << " minutes" << endl;

        if (absdiff > 60)
        {
          result = StateAdjusting;
        }
        else
        {
          result = StateAdvancingMinute;
        }
      }
    }
    break;

  default:
    Serial << "Idle: Ignoring event " << (unsigned)event << endl;
  }
  
  if ((result == StateIdle) && ((clock_hour == 255) || (clock_min == 255)))
  {
    Serial << "Idle: Clock is uncalibrated" << endl;

    result = StateCalibrating;
  }

  return result;
}


//---------------------------------------------------------------------------
// Event handler used while calibrating
//
// This is called while we're trying to figure out where the hands are
EVENTHANDLER_PROTO(state_Calibrating)
{
  State result = StateCalibrating;

  switch (event)
  {
  case EventInit:
    // Init the state: stop the minute motor, start the adjustment motor in
    // the direction that most likely lets us hit the 12 o'clock mark as
    // fast as possible.
    // TODO: This would work a lot better if the 12-hour sensor was on
    // TODO: during the hours of 6 to 12, instead of only around 12.
    motor_min.Stop();
    motor_adj.Rotate(sensor_h.IsOff() ? FishduinoMotor::CCW : FishduinoMotor::CW); // Not really useful

    Serial << "Calibrating: Init" << endl;
    break;

  case EventHourDown:
    // We just passed the midnight position in backwards direction. Because
    // of mechanical tolerances and inertia, we don't want to stop
    // calibrating right now; change to forward direction first. The main
    // loop should catch the sensor change almost right away.
    if (clock_hour != 255)
    {
      Serial << "Reversing to optimize calibration" << endl;

      motor_adj.Clockwise();
    }
    break;

  case EventHourUp:
    // We've just sensed the minute hand going across the top of the hour.
    // If the main loop figured out where the hour hand was, we're 
    // calibrated and we can start adjusting the clock to the current time.
    if (clock_hour != 255)
    {
      Serial << "Calibrating: Calibration complete, adjusting now." << endl;
      result = StateAdjusting;
    }
    break;

  default:
    Serial << "Calibrating: Ignoring event " << (unsigned)event << endl;
  }

  return result;
}


//---------------------------------------------------------------------------
// Event handler used while handling elapsed time or small time differences
EVENTHANDLER_PROTO(state_AdvancingMinute)
{
  State result = StateAdvancingMinute;

  int diff = GetTimeDiff();

  // If there's nowhere to go, switch to the Idle state.
  // This happens not only when we reach the actual time but also when we
  // lose track of the actual time or clock time.
  // We can check for this even before initializing
  if (!diff)
  {
    Serial << "Minute: No time difference, switching to Idle" << endl;
    result = StateIdle;
  }
  else
  {
    switch (event)
    {
    case EventInit:
      // Init the state: start the minute motor in the desired direction
      motor_adj.Stop();
      motor_min.Rotate(diff < 0 ? FishduinoMotor::CCW : FishduinoMotor::CW);

      Serial << "Minute: Init; time difference is " << diff << endl;
      break;

    case EventMinuteUp:
    case EventMinuteDown:
      // Minute motor went around; nothing to do but notify developer
      Serial << "Minute: Time difference is " << diff << endl;
      break;

    default:
      Serial << "Minute: Ignoring event " << (unsigned)event << endl;
    }
  }

  return result;
}


//---------------------------------------------------------------------------
// Event handler used when adjusting time, e.g. to change time zone
EVENTHANDLER_PROTO(state_Adjusting)
{
  State result = StateAdjusting;

  int hourdiff = GetHourDiff();

  // If there's nowhere to go, go to the Idle state.
  // This happens not only when we reach the actual hour but also when we
  // lose track of the actual time or clock time.
  // We can check for this even before initializing
  if (!hourdiff)
  {
    Serial << "Adjusting: No hour difference, switching to Minute" << endl;
    result = StateAdvancingMinute;
  }
  else
  {
    switch (event)
    {
    case EventInit:

      if (abs(hourdiff) > 1)
      {
        motor_min.Stop();
        motor_adj.Rotate(hourdiff > 0 ? FishduinoMotor::CW : FishduinoMotor::CCW);

        Serial << "Adjusting: Init" << endl;
      }
      else
      {
        Serial << "Adjusting: Init: time difference less than an hour, switching to Minute" << endl;

        result = StateAdvancingMinute;
      }
      break;

    case EventHourDown:
      // If we reached the desired hour but we're going backwards, reverse
      // the direction to make up for mechanical inaccuracy and inertia
      // The main loop will catch the sensor again in forward direction.
      if (hourdiff == 0)
      {
        Serial << "Adjusting: Reversing direction to optimize adjustment" << endl;
        motor_adj.Clockwise();
      }
      break;

    case EventHourUp:
      // Check if we reached the desired hour
      if (hourdiff == 0)
      {
        Serial << "Adjusting: Reached hour, switching to Minute" << endl;

        result = StateAdvancingMinute;
      }
      break;

    default:
      Serial << "Adjusting: Ignoring event " << (unsigned)event << endl;
    }
  }

  return result;
}


//---------------------------------------------------------------------------
// Error state event handler
EVENTHANDLER_PROTO(state_Error)
{
  Serial << "ERROR" << endl;

  for(;;);

  // unreachable
  return StateError;
}


/////////////////////////////////////////////////////////////////////////////
// ARDUINO TOP LEVEL FUNCTIONS
/////////////////////////////////////////////////////////////////////////////


//---------------------------------------------------------------------------
// Arduino setup
void setup()
{
  Serial.begin(115200);
  gps.Attach();

  Serial << "Running!\n";

  // We want to use the table by indexing it with a state, so make sure
  // that the data is in the right place. If not, it means someone changed
  // the enum without changing the table, or vice versa
  //
  // First, make sure the table is the right size
  if (ARR_LEN(statetable) != (size_t)StateNum)
  {
    Serial << "State table length inconsistency detected" << endl;
    for(;;);
  }

  // Now check that each entry has a state that corresponds to its index
  for (unsigned u = 0; u < (unsigned)StateNum; u++)
  {
    if (statetable[u].state != (State)u)
    {
      Serial << "State table inconsistency detected. Expected " << u << " found " << statetable[u].state << endl;
      for(;;);
    }
  }
}


//---------------------------------------------------------------------------
// Arduino loop
void loop()
{
  static State cur_state = StateIdle;
  static bool firsttimeinstate = true;

  Event event;
  bool gotsensors = false;

  for (;;)
  {
    event = EventNone;

    if (event == EventNone)
    {
      if (firsttimeinstate)
      {
        Serial << "State changed to: " << statetable[cur_state].name << endl;
        event = EventInit;
        firsttimeinstate = false;

        // Force getting the sensors
        gotsensors = false;
      }
    }

    if (event == EventNone)
    {
      // Update the inputs if we haven't already done so during this loop
      if (!gotsensors)
      {
        // Depending on your hardware, you may need to adjust the debounce 
        // parameters for this call.
        fishduino.UpdateInputs(2);
      }

      // Check if we need to generate a minute-up or minute-down event
      if (event == EventNone)
      {
        if ((sensor_m.IsOff()) && (sensor_m.WasOn() && (motor_min.GetState() == FishduinoMotor::CW)))
        {
          // The minute motor is running clockwise and the sensor went off.
          Serial << "Minute Up" << endl;

          event = EventMinuteUp;

          ClockMinuteUp();
        }
        else if ((sensor_m.IsOn()) && (sensor_m.WasOff()) && (motor_min.GetState() == FishduinoMotor::CCW))
        {
          // The minute motor is running counterclockwise and the sensor went on.
          Serial << "Minute Down" << endl;

          event = EventMinuteDown;

          ClockMinuteDown();
        }
      }

      // Check if we need to generate an hour-up or hour-down event
      // Note, the hour events are only generated while adjusting; if we
      // would also generate them when the minute motor is running, the
      // event handler would get two events for the same clock change, and
      // the hour sensor may be inaccurate because of mechanical limitations,
      // so it might happen a minute too early or too late, so we have to
      // ignore it during normal time keeping.
      if (event == EventNone)
      {
        if ((sensor_h.IsOff()) && (sensor_h.WasOn()) && (motor_adj.GetState() == FishduinoMotor::CW))
        {
          // The adjustment motor is running clockwise and the sensor went off.
          Serial << "Hour Up" << endl;

          event = EventHourUp;

          if (sensor_h12.IsOn())
          {
            clock_hour = 0;
          }
          else
          {
            ClockHourUp();
          }

          clock_min = 0;
        }
        else if ((sensor_h.IsOn()) && (sensor_h.WasOff()) && (motor_adj.GetState() == FishduinoMotor::CCW))
        {
          // The adjustment motor is running counterclockwise and the sensor went on.
          Serial << "Hour Down" << endl;

          event = EventHourDown;

          if (sensor_h12.IsOn())
          {
            clock_hour = 11;
          }
          else
          {
            ClockHourDown();
          }

          clock_min = 59;
        }
      }
    }

    if (event == EventNone)
    {
      // Check if the actual time changed
      // This may take a while, so we should only do this if the motors are
      // off, or we might miss some sensor changes
      if ((motor_adj.GetState() == FishduinoMotor::STOP) && (motor_min.GetState() == FishduinoMotor::STOP))
      {
        tmElements_t tm;

        time_t tt;
        
        // TODO: Currently hard-coded for Pacific Time (-8)
        // Should read the time zone from EEPROM, adjust for DST on the 
        // correct dates etc.
        if (!gps.GetLocalTime(-8 * 60 * 60, &tt, 1500UL, 5 * 60 * 1000UL))
        {
          // We don't have an actual time
          if ((actual_hour != 255) || (actual_min != 255))
          {
            Serial << "GPS lost" << endl;
            actual_hour = actual_min = 255;

            event = EventTime;
          }
        }
        else
        {
          breakTime(tt, tm);

          byte th = tm.Hour % 12; // Use our 0..11 standard

          if ((actual_hour == 255) || (actual_hour != th) || (actual_min == 255) || (actual_min != tm.Minute))
          {
            Serial << "Time changed" << endl;

            actual_hour = th;
            actual_min = tm.Minute;

            event = EventTime;
          }
        }
      }
    }

    if (event == EventNone)
    {
      // TODO: Handle serial input
    }

    State new_state = cur_state;

    // If an event has been set, execute the current state's event handler
    if (event != EventNone)
    {
      Serial << "Actual: " << actual_hour << ":" << actual_min << endl;
      Serial << "Clock:  " << clock_hour  << ":" << clock_min << endl;

      new_state = statetable[cur_state].func(event);

      // If the adjustment motor is on, forget the clock minutes
      if ((motor_adj.GetState() != FishduinoMotor::STOP) && (new_state == cur_state))
      {
        clock_min = 255;
      }
    }

    static unsigned long lastoutputupdatetime;
    unsigned long ul = millis();

    // If an event was set or if it's been a while since the outputs were
    // updated, do it now.
    if ((event != EventNone) || (ul - lastoutputupdatetime > 5000))
    {
      lastoutputupdatetime = ul;
      fishduino.UpdateOutputs();
    }

    // If the state was changed by the last event handler, flag it
    if (new_state != cur_state)
    {
      cur_state = new_state;
      firsttimeinstate = true;
    }
    else if (event == EventNone)
    {
      // No event was handled
      break;
    }
  }
}


/////////////////////////////////////////////////////////////////////////////
// END
/////////////////////////////////////////////////////////////////////////////
