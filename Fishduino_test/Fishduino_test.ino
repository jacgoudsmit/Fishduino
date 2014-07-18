
#include <Fishduino.h>

Fishduino ft;

void setup()
{
  Serial.begin(115200);
  Serial.println("Hello!");

  if (!ft.Setup())
  {
    Serial.println("Setup error!");
  }
}

void loop()
{
  byte data[ft.MaxInterfaces];

  ft.GetInputs( sizeof(data), data);
  ft.SetOutputs(sizeof(data), data);

  for (unsigned u = 0; u < sizeof(data); u++)
  {
    byte b = data[u];

    for (unsigned v = 0; v < 8; v++)
    {
      Serial.print(((b & (1 << v)) != 0) ? '1' : '0');
    }

    Serial.print(' ');
  }
  Serial.println();

  for (int i = 0; i < 2; i++)
  {
    Serial.print(ft.GetAnalog(i));
    Serial.print(' ');
  }
  Serial.println();

  delay(100);
}

