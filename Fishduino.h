/*
  Schematic at http://www.fischertechnik-fans.de/Images/pischaltplan.gif
  Some code based on an article in http://www.ftcommunity.de/ftpedia_ausgaben/ftpedia-2014-1.pdf

  20-pole ribbon cable pinout. Directions relative to host (Arduino).
  The pin numbers on the PCB in the 30520 interface are printed
  backwards (20<->1, 19<->2 etc).
  The analog inputs (EX and EY) run from the Fischertechnik side of the
  interface to the computer, and then are looped back to two 556 timers on
  the interface. Two loopback wires must be installed (between pins 5 and 7,
  and between pins 6 and 8) on the Arduino side of the grey ribbon cable to
  make the analog inputs work as expected. It may be possible to connect
  those pins directly to two analog input pins on the Arduino, but this is
  not supported by the library.

  +---------------+--------------+---------+-----+---------------------------
  | Pin number    | Pin number   | Default | Dir | Function
  | according to  | on Arduino   | Arduino |     |
  | interface PCB | side of grey | pin     |     |
  |               | interface    |         |     |
  |               | cable        |         |     |
  +---------------+--------------+---------+-----+---------------------------
  |      20       |       1      |   GND   | GND | GND
  |      19       |       2      |         |     | GND
  |      18       |       3      |    2    | IN  | DATA/COUNT IN
  |      17       |       4      |         |     | Not connected
  |      16       |       5      |         |     | Connect to pin 14/7
  |      15       |       6      |         |     | Connect to pin 13/8
  |      14       |       7      |         |     | Connect from pin 16/5
  |      13       |       8      |         |     | Connect from pin 15/6
  |      12       |       9      |    3    | OUT | TRIGGER X
  |      11       |      10      |    4    | OUT | TRIGGER Y
  |      10       |      11      |    5    | OUT | DATA OUT
  |       9       |      12      |    6    | OUT | CLOCK
  |       8       |      13      |    7    | OUT | LOAD OUT
  |       7       |      14      |    8    | OUT | LOAD IN
  |       6       |      15      |         |     | Not connected
  |       5       |      16      |         |     | Not connected
  |       4       |      17      |         |     | Not connected
  |       3       |      18      |         |     | Not connected
  |       2       |      19      |         |     | GND
  |       1       |      20      |         |     | GND
  +---------------+--------------+---------+---------------------------------

  NOTE: expansion devices aren't supported yet.
*/

#include <Arduino.h>

#include "TimerOne.h"   // https://github.com/PaulStoffregen/TimerOne

class Fishduino
{
protected:
  // These values are used to index the pin array
  enum 
  {
                              //                Interface pin / Arduino pin (default)
    pin_INPUT,                // DATA/COUNT IN  3  / 2
    pin_TRIGGERX,             // TRIGGER X      9  / 3
    pin_TRIGGERY,             // TRIGGER Y      10 / 4
    pin_DATAOUT,              // DATA OUT       11 / 5
    pin_CLOCK,                // CLOCK          12 / 6
    pin_LOADOUT,              // LOAD OUT       13 / 7
    pin_LOADIN,               // LOAD IN        14 / 8

    // Helper
    pin_NUM                   // Number of pins used
  };

  // Default ISR period in microseconds; e.g. 10000=10ms=100Hz
  enum { DefaultInterval = 3000 };

  // Array that defines which interface pin is connected to which Arduino pin
  // See enum definition above for indexes.
  // This is initialized at construction time and not changed afterwards.
  byte m_pin[pin_NUM];

  // This tracks the outputs
  volatile byte m_outdata;

  // This tracks the digital inputs
  volatile byte m_indata;

  // Data used by the Interrupt Service Routine.
  // NOTE: There is only one ISR which handles all instances of this class.
  bool m_allowisr;                      // False=skip processing for instance
  static Fishduino * volatile m_list;   // List of all instances
  Fishduino * volatile m_next;          // Link to next instance

private:
  //-------------------------------------------------------------------------
  // Constructor helper
  void _Fishduino(
    byte pin_datacountin,
    byte pin_triggerx,
    byte pin_triggery,
    byte pin_dataout,
    byte pin_clock,
    byte pin_loadout,
    byte pin_loadin)
  {
    // Initialize Arduino pin numbers for each pin on the interface
    m_pin[pin_INPUT]    = pin_datacountin;
    m_pin[pin_TRIGGERX] = pin_triggerx;
    m_pin[pin_TRIGGERY] = pin_triggery;
    m_pin[pin_DATAOUT]  = pin_dataout;
    m_pin[pin_CLOCK]    = pin_clock;
    m_pin[pin_LOADOUT]  = pin_loadout;
    m_pin[pin_LOADIN]   = pin_loadin;

    // Turn all motors off just in case
    m_outdata = 0;
  }

public:
  //-------------------------------------------------------------------------
  // Constructor
  Fishduino(
    byte pin_datacountin,
    byte pin_triggerx,
    byte pin_triggery,
    byte pin_dataout,
    byte pin_clock,
    byte pin_loadout,
    byte pin_loadin)
  {
    _Fishduino(
      pin_datacountin,
      pin_triggerx,
      pin_triggery,
      pin_dataout,
      pin_clock,
      pin_loadout,
      pin_loadin);
  }

public:
  //-------------------------------------------------------------------------
  // Simpler constructor for when interface is on consecutive Arduino pins
  Fishduino(
    byte startpin = 2)
  {
    _Fishduino(
      startpin,
      startpin + 1,
      startpin + 2,
      startpin + 3,
      startpin + 4,
      startpin + 5,
      startpin + 6);
  }

public:
  //-------------------------------------------------------------------------
  // Destructor
  virtual ~Fishduino()
  {
    // Disable interrupts so the ISR won't interfere
    noInterrupts();

    // Remove our instance from the linked list
    if (m_list == this)
    {
      // Trivial case: We're at the top of the list
      m_list = m_next;
    }
    else
    {
      // Walk through the list and find us
      for (Fishduino volatile *p = m_list; p; p = p->m_next)
      {
        if (p->m_next == this)
        {
          // Found our predecessor, remove us
          p->m_next = m_next;
          break;
        }
      }
    }

    // Re-enable interrupts before calling external functions
    // The linked list is valid at this point so if the ISR gets called,
    // it won't handle our instance anyway which is OK.
    interrupts();

    // If we were the last remaining instance, detach the ISR.
    if (!m_list)
    {
      Timer1.detachInterrupt();
    }

  }

public:
  //-------------------------------------------------------------------------
  // Initialize
  //
  // The interval is in microseconds; it's only used for the first instance.
  //
  // NOTE: not thread-safe
  void Setup(
    unsigned long interval = DefaultInterval)
  {
    // Prevent interrupts from screwing things up
    // Need to do this before assigning pins, in case pins are shared
    // between instances (this is not tested but should be possible).
    noInterrupts();

    // Initialize pins
    for (unsigned u = 0; u < pin_NUM; u++)
    {
      pinMode(m_pin[u], u == pin_INPUT ? INPUT : OUTPUT);
    }

    // Initialize the local data and the output pins
    m_outdata = 0; // All outputs off
    UpdateOutputs();
    UpdateInputs();
    digitalWrite(m_pin[pin_TRIGGERX], HIGH);
    digitalWrite(m_pin[pin_TRIGGERY], HIGH);

    // Update linked list of all instances of our class
    // This has to be done while interrupts are off
    if (interval)
    {
      m_next = m_list;                  // Demote the head of the list
      m_list = this;                    // Make this instance the head
    }

    // The list is now in a known state; enable interrupts again before we
    // call any external functions
    m_allowisr = true;

    interrupts();

    // If we are the first instance, initialize the timer
    if ((interval) && (!m_next))
    {
      Timer1.initialize(interval);
      Timer1.attachInterrupt(static_isr);
    }
  }

public:
  //-------------------------------------------------------------------------
  // Update the outputs of the interface
  virtual void UpdateOutputs()
  {
    byte b = m_outdata;

    digitalWrite(m_pin[pin_LOADOUT], LOW);

    for (unsigned u = 0; u < 8; u++, b <<= 1)
    {
      digitalWrite(m_pin[pin_CLOCK], LOW);
      digitalWrite(m_pin[pin_DATAOUT], (b & 0x80) != 0);
      digitalWrite(m_pin[pin_CLOCK], HIGH);
    }

    digitalWrite(m_pin[pin_LOADOUT], HIGH);
    digitalWrite(m_pin[pin_LOADOUT], LOW);

    // At this point:
    // - CLOCK is HIGH
    // - DATA OUT is LOW
    // - LOAD OUT is LOW
  }

public:
  //-------------------------------------------------------------------------
  // Read the digital inputs from the interface
  //
  // This stops the analog timers.
  virtual void UpdateInputs()
  {
    byte data = 0;

    // Switch the input chip to parallel mode and clock it to load the inputs
    digitalWrite(m_pin[pin_LOADIN], HIGH);
    digitalWrite(m_pin[pin_CLOCK], LOW);
    digitalWrite(m_pin[pin_CLOCK], HIGH);
    digitalWrite(m_pin[pin_LOADIN], LOW);

    // Read the inputs
    for (unsigned u = 0; u < 8; u++)
    {
      data <<= 1;

      data |= (digitalRead(m_pin[pin_INPUT]) == LOW);

      digitalWrite(m_pin[pin_CLOCK], LOW);

      digitalWrite(m_pin[pin_CLOCK], HIGH);
    }

    m_indata = data;

    // At this point:
    // - CLOCK is high
    // - LOAD IN is LOW
  }

protected:
  //-------------------------------------------------------------------------
  // Interrupt Service Routine (thiscall version)
  //
  // This gets called on a regular basis for each existing instance.
  virtual void isr()
  {
    UpdateInputs();
    UpdateOutputs();
  }

protected:
  //-------------------------------------------------------------------------
  // Interrupt Service Routine (static version)
  //
  // This gets called by the timer interrupt
  static void static_isr()
  {
    for (Fishduino *p = m_list; p; p = p->m_next)
    {
      if (p->m_allowisr)
      {
        p->isr();
      }
    }
  }

public:
  //-------------------------------------------------------------------------
  // Get an analog input
  //
  // This takes a while (up to ~2.5ms) so this is done outside the ISR.
  // The R/C circuit in the interface is apparently dimensioned to generate
  // a maximum interval of about 2550 microseconds, so we simply measure the
  // time it takes in microseconds, and divide the value by 10.
  byte Analog(                          // Returns value, 255=not connected
    byte index)                         // 0=X, 1=Y
  {
    byte trigger = m_pin[index ? pin_TRIGGERY : pin_TRIGGERX];

    // Turn interrupts off, this function takes a while and shouldn't be done
    // inside an ISR
    m_allowisr = false;

    digitalWrite(trigger, LOW);
    digitalWrite(trigger, HIGH);

    unsigned long n;
    unsigned long t = 0;

    // Empirical evidence shows that the timer output stays low
    // between approximately 240 and 2550 microseconds.
    // There is no official documentation about this, but it's
    // probably not a coincidence, so wait for a maximum of
    // 2550 microseconds and divide the result by 10 to get a
    // value between 20 and 255.
    for (n = micros(); digitalRead(m_pin[pin_INPUT]) == LOW; )
    {
      t = micros() - n;
      if (t > 2550)
      {
        break;
      }
    }

    // Enable interrupts again
    m_allowisr = true;

    return t / 10;
  }

public:
  //-------------------------------------------------------------------------
  // Stop all motors immediately
  void ResetOutputs()
  {
    m_outdata = 0;
  }

public:
  //-------------------------------------------------------------------------
  // Update all outputs
  void AllOutputs(
    byte b)
  {
    m_outdata = b;
  }

public:
  //-------------------------------------------------------------------------
  // Change digital output
  void Out(
    byte index,                         // 0..7
    byte lohi)
  {
    if (index < 8)
    {
      if (lohi)
      {
        m_outdata |= (1 << index);
      }
      else
      {
        m_outdata &= ~(1 << index);
      }
    }
  }

public:
  byte AllInputs()
  {
    return m_indata;
  }

public:
  //-------------------------------------------------------------------------
  // Get the given digital input
  bool In(
    byte index)                         // Digital input number (0..7)
  {
    bool result;

    noInterrupts();

    result = (0 != ((m_indata >> index) & 1));

    interrupts();

    return result;
  }
};

