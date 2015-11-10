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


// Uncomment this to generate GPS logging
//#define DEBUG_GPS

#include <Arduino.h>

#include <SoftwareSerial.h>
#include <TinyGPS.h> // http://arduiniana.org/libraries/tinygps/
#include <Time.h> // http://www.pjrc.com/teensy/td_libs_Time.html


/////////////////////////////////////////////////////////////////////////////
// MACROS
/////////////////////////////////////////////////////////////////////////////


#ifdef DEBUG_GPS
#define GPSLOG(x) Serial.print(x)
#define GPSLOGLN(x) Serial.println(x)
#else
#define GPSLOG(x)
#define GPSLOGLN(x)
#endif


/////////////////////////////////////////////////////////////////////////////
// GPS TIME
/////////////////////////////////////////////////////////////////////////////


class GPSTime
{
public:
  //-------------------------------------------------------------------------
  // GPS state type
  enum GPSState 
  { 
    GPSStateUnknown,                    // Default state
    GPSStateOnline,                     // Port is open, no data yet
    GPSStateData,                       // Received data, nothing valid yet
    GPSStateValidData,                  // Received valid data, no time/date
    GPSStateGotTime,                    // Got time, no date yet
    GPSStateGotDate                     // Got time and date
    // To be expanded...
    
  };


#ifndef NDEBUG
public:
#else
protected:
#endif
  //-------------------------------------------------------------------------
  // Member data
  GPSState          m_gps_state;
  SoftwareSerial    m_gps_serial;
  TinyGPS           m_gps;


public:
  //-------------------------------------------------------------------------
  // Constructor
  GPSTime(
    byte gps_inpin,                     // RxD for GPS serial (to   GPS TxD)
    byte gps_outpin)                    // TxD for GPS serial (from GPS RxD)
  : m_gps_state(GPSStateUnknown)
  , m_gps_serial(gps_inpin, gps_outpin)
  , m_gps()
  {
    // Nothing to do here
  }

public:
  //-------------------------------------------------------------------------
  // Attach GPS and turn it on
  void Attach()
  {
    // Factory reset
    //m_gps_serial.println("$PMTK104*37");
    
    // send GPRMC and GPGGA
    m_gps_serial.println(F("$PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28"));
    
    // send updates once per second
    m_gps_serial.println(F("$PMTK220,1000*1F"));
    
    // send updates 5x per second
    //m_gps_serial.println(F("$PMTK220,200*2C"));
    
    m_gps_state = GPSStateOnline;
  }

public:
  //-------------------------------------------------------------------------
  // Disconnect GPS (turn it off)
  void Detach()
  {
    // Nothing for now
    // (In the future, commands may be sent to the GPS module here)
    m_gps_state = GPSStateUnknown;
  }

public:
  //-------------------------------------------------------------------------
  // Get current state
  GPSState GetState()
  {
    return m_gps_state;
  }

public:
  //-------------------------------------------------------------------------
  // Get data from GPS until an NMEA sentence is received.
  //
  // Returns true if an update was received, false on timeout
  // NOTE: Any servos should be detached while doing this.
  bool GetGPS(unsigned long timeout)
  {
    m_gps_serial.begin(9600);
    
    bool result = false;
    unsigned long ts = millis();
    
    for(;;)
    {
      // Get characters from GPS serial port and feed them to the GPS parser.
      // If there's nothing to read, test for timeout.
      unsigned numavailable = m_gps_serial.available();
      if (numavailable)
      {
        if (m_gps_state < GPSStateData)
        {
          m_gps_state = GPSStateData;
        }

        bool gotsentence = false;

        // Stuff as much data into the GPS parser as possible to avoid losing
        // data from the serial port
        do
        {
          GPSLOG("."); // Indicate that we're processing the receive buffer
          for (unsigned i = 0; i < numavailable; i++)
          {
            char c = m_gps_serial.read();
            //GPSLOG(c); // Dump incoming data
          
            if (m_gps.encode(c))
            {
              GPSLOG("*"); // Indicate that we got the end of a sentence
              gotsentence = true;
              
              // Note, we're not stopping here; keep reading until the GPS
              // is "quiet" for a while.
              // Experiments have shown that if we break out here, we often
              // still don't have valid data because the GPS sends so much
              // stuff that TinyGPS doesn't use. It's much more reliable to
              // simply keep going until we detect the interval between
              // updates instead.
            }
          }
        
          delay(1); // This seems to prevent framing errors
          
          // Check if there's more data available now
          numavailable = m_gps_serial.available();
          
          // Quit the loop between updates from the GPS
        } while (numavailable);
        
        GPSLOGLN("Parsing...");
        
        if (gotsentence)
        {
          if (m_gps_state < GPSStateValidData)
          {
            m_gps_state = GPSStateValidData;
          }
          
          unsigned long date;
          unsigned long time;
          
          m_gps.get_datetime(&date, &time, NULL);
          
          if ((time != TinyGPS::GPS_INVALID_TIME) && (m_gps_state < GPSStateGotTime))
          {
            m_gps_state = GPSStateGotTime;
            
            if ((date != TinyGPS::GPS_INVALID_DATE) && (m_gps_state < GPSStateGotDate))
            {
              m_gps_state = GPSStateGotDate;
            }
          }
            
          if (m_gps_state >= GPSStateGotDate)
          {
            GPSLOGLN("Got date and time");          
            result = true;
            break;
          }
        }
      }
      else
      {
        // If we received nothing, check for timeout
        if (millis() - ts >= timeout)
        {
          GPSLOGLN("GPS timeout");
          break;
        }
      }
    }
    
    m_gps_serial.end();
    
    return result;
  }
  
public:
  //-------------------------------------------------------------------------
  // Get UTC time from the GPS module if possible
  bool GetUTCTime(
    int *pyear,
    byte *pmonth,
    byte *pday,
    byte *phour,
    byte *pminute,
    byte *psecond,
    unsigned long timeout = 1500,
    unsigned long max_age = 500)
  {
    bool result = false;
    
    if (GetGPS(timeout))
    {
      unsigned long age;

      m_gps.crack_datetime(pyear, pmonth, pday, phour, pminute, psecond, NULL, &age);

      GPSLOG("Got UTC. Age=");
      GPSLOGLN(age);
      
      if (age < max_age)
      {
        GPSLOGLN("Success UTC");      
        result = true;
      }
      else
      {
        GPSLOGLN("GPS lost");
      }
    }
    else
    {
      GPSLOGLN("GPS not received yet");
    }
    
    return result;
  }

public:
  //-------------------------------------------------------------------------
  // Get UTC time from GPS as time_t type.
  //
  // If something went wrong, the function returns 0.
  bool GetUTCTime(
    time_t *ptime,
    unsigned long timeout = 1500,
    unsigned long max_age = 500)
  {
    bool result;
    tmElements_t te;
    int year;

    result = GetUTCTime(
      &year,
      &te.Month,
      &te.Day,
      &te.Hour,
      &te.Minute,
      &te.Second,
      timeout,
      max_age);

    if (result && ptime)
    {
      te.Year = CalendarYrToTm(year);
      *ptime = makeTime(te);

      GPSLOG("Current time is: ");
      GPSLOG(te.Year);
      GPSLOG("/");
      GPSLOG(te.Month);
      GPSLOG("/");
      GPSLOG(te.Day);
      GPSLOG(" ");
      GPSLOG(te.Hour);
      GPSLOG(":");
      GPSLOG(te.Minute);
      GPSLOG(":");
      GPSLOG(te.Second);   
      GPSLOG(" Got UTC=");
      GPSLOGLN(result);
    }

    return result;
  }

public:
  //-------------------------------------------------------------------------
  // Get local time from GPS
  //
  // If something went wrong, the function returns 0.
  bool GetLocalTime(
    long timezone,                      // Time offset in seconds
    time_t *ptime,                      // Output time
    unsigned long timeout = 1000,
    unsigned long max_age = 500)
  {
    bool result = GetUTCTime(ptime, timeout, max_age);
    
    if (result && ptime)
    {
      *ptime += (time_t)timezone;
    }
    
    GPSLOG("Local time=");
    GPSLOGLN(*ptime);
    
    return result;
  }


/*
public:
  //-------------------------------------------------------------------------
  // Static version of the above, needed by automatic time sync below
  static time_t StaticLocalTime(void *userParm)
  {
    time_t result = 0;
    GPSTime *p = (GPSTime *)userParm;

    if (p)
    {
      // If the time has not been set yet, wait longer to give up
      // We can't call timeStatus() here, it would cause infinite recursion
      // so we remember our previous result
      static unsigned long timeout = 3000;

      result = p->GetLocalTime(timeout);
      
      // If GPS could not be synced, try again soon
      if (!result)
      {
        // If that didn't work, try again after a short time
        setSyncInterval(5);
        
        // Wait extra long to get data
        timeout = 3000;
      }
      else
      {
        // If it worked, ask again after 5 minutes
        setSyncInterval(300);
        
        // No need to try very hard
        timeout = 1000;
      }
    }
    
    GPSLOGLN("Auto update called");
    
    return result;
  }

public:
  //-------------------------------------------------------------------------
  // Enable synchronization of the system time in the Time library
  //
  // After calling this, you can use the Time functions to get the time.
  // You should make sure that any servos are always detached whenever you
  // call a Time function; if not, the incoming serial traffic from the GPS
  // module will interfere with the operation of the servos.
  void EnableTimeSync(void)
  {
    setSyncProvider(StaticLocalTime, this);
  }

public:
  //-------------------------------------------------------------------------
  // Disable automatic time synchronization
  void DisableTimeSync(void)
  {
    setSyncProvider(NULL, NULL);
  }
*/
};