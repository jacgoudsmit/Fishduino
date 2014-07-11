int ledpin = 13;

int datain = 2;
int trigger[2] = { 3, 4 };
int dataout = 5;
int clock = 6;
int loadout = 7;
int loadin = 8;

int m[9]; // 0 not used
int e[9]; // 0 not used
int a[2];

#define EX a[0]
#define EY a[1]

void setup()
{
  Serial.begin(115200);
  Serial.println("Hello!");

  pinMode(ledpin, OUTPUT);

  pinMode(datain, INPUT);
  pinMode(trigger[0], OUTPUT);
  pinMode(trigger[1], OUTPUT);
  pinMode(dataout, OUTPUT);
  pinMode(clock, OUTPUT);
  pinMode(loadout, OUTPUT);
  pinMode(loadin, OUTPUT);
  
  digitalWrite(clock, LOW);
  digitalWrite(dataout, LOW);
  digitalWrite(loadout, LOW);
  digitalWrite(datain, HIGH);
  digitalWrite(loadin, LOW);
  digitalWrite(trigger[0], HIGH);
  digitalWrite(trigger[1], HIGH);
  
  digitalWrite(ledpin, LOW);
}

void loop()
{
  getinputs();
  for (int i = 1; i <= 8; i++)
  {
    Serial.print(e[i]);
    Serial.print(' ');
  }
  Serial.println();

  getanaloginputs();
  for (int i = 0; i < 2; i++)
  {
    Serial.print(a[i]);
    Serial.print(' ');
  }
  Serial.println();

  for (int i = 1; i <= 8; i++)
  {
    m[i] = e[i];
  }
  updateoutputs();
  
  delay(100);
}

#define D(x) //Serial.println(x); delay(1000);

void getinputs()
{
  digitalWrite(loadin, HIGH);
  D("parallel")
  digitalWrite(clock, LOW);
  D("clock low")
  digitalWrite(clock, HIGH);
  D("clock high")
  digitalWrite(loadin, LOW);
  D("serial")
  
  for (int i = 8; i > 0; i--)
  {
    e[i] = digitalRead(datain) == LOW;
    D(e[i])
    digitalWrite(clock, LOW);
    D("clock low")
    digitalWrite(clock, HIGH);
    D("clock high")
  }
}

void getanaloginputs()
{
  for (int i = 0; i < 2; i++)
  {
    digitalWrite(trigger[i], LOW);
    digitalWrite(trigger[i], HIGH);
    
    unsigned long n = micros();
    unsigned long t = 0;

    // Empirical evidence shows that the timer output stays low
    // between approximately 240 and 2550 microseconds.
    // There is no official documentation about this, but it's
    // probably not a coincidence, so wait for a maximum of
    // 2550 microseconds and divide the result by 10 to get a
    // value between 20 and 255.
    while (digitalRead(datain) == LOW)
    {
      t = micros() - n;
      if (t > 2550)
      {
        break;
      }
    }
    
    a[i] = t / 10;
  }  
}

void updateoutputs()
{
  // Disable strobe to prevent interference
  digitalWrite(loadout, LOW);

  // Shift outputs right to left
  for (int i = 8; i > 0; i--)
  {
    digitalWrite(clock, LOW);
    digitalWrite(dataout, m[i]);
    digitalWrite(clock, HIGH);
  }

  digitalWrite(loadout, HIGH);
  digitalWrite(loadout, LOW);
}

