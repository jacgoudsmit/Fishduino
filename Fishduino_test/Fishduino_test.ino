
#include <TimerOne.h>

#include "Fishduino.h"

Fishduino ft(2);

void setup()
{
  Serial.begin(115200);
  Serial.println("Hello!");

  ft.Setup();
}

void loop()
{
  for (int i = 0; i < 8; i++)
  {
    bool b = ft.In(i);

    Serial.print(b ? '1' : '0');

    ft.Out(i, b);
  }
  Serial.println();

  for (int i = 0; i < 2; i++)
  {
    Serial.print(ft.Analog(i));
    Serial.print(' ');
  }
  Serial.println();

  delay(100);
}

