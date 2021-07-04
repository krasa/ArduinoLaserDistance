//based on https://github.com/BaraVu/Arduino_laser_distance

#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>

LiquidCrystal_I2C lcd(0x27, 16, 2); // set the LCD address to 0x27 for a 16 chars and 2 line display

//my laser module: RX - light blue, TX white
//(Rx,Tx)
SoftwareSerial laserSerial(9, 8);

const boolean useButton = false;
const byte buttonPin = 6;
boolean manualMode = false;
volatile double lastMeasure = 0.0;
volatile double tare = 0.0;

String message;

void measure(char string, int measureDelay);

void status();

void open_head();

void close_head();

void printSerialFromLaser(String s);

void displayLcd(String string);

void interrupted();

void manualProcessing();

void setup()
{
  if (useButton)
  {
    pinMode(buttonPin, INPUT);
    attachInterrupt(digitalPinToInterrupt(buttonPin), interrupted, RISING);
  }

  laserSerial.begin(19200);
  Serial.begin(9600);

  lcd.init();
  lcd.backlight();

  if (useButton && digitalRead(buttonPin) == HIGH)
  {
    manualMode = true;
  }

  status();
  delay(1000);
}

volatile int buttonHold = 0;

void normalMeasure()
{
  Serial.println("measure");
  measure('D', 1000);
}
void fastMeasure()
{
  Serial.println("fast measure");
  measure('F', 500);
}
void slowMeasure()
{
  Serial.println("slow measure");
  measure('M', 4000);
}
void loop()
{
  if (manualMode)
  {
    manualProcessing();
  }
  else
  {
    // normalMeasure();
    // slowMeasure();
    fastMeasure();
    // delay(400);

    if (useButton)
    {
      if (digitalRead(buttonPin) == HIGH)
      {
        buttonHold++;
      }
      else
      {
        buttonHold = 0;
      }
      if (buttonHold >= 2)
      {
        tare = 0.0;
      }
    }

    if (Serial.available())
    {
      manualMode = true;
      displayLcd("manualni mod");
    }
  }
}

void manualProcessing()
{
  if (Serial.available())
  {
    // wait a bit for the entire message to arrive
    delay(100);

    while (Serial.available() > 0)
    {
      char blt = Serial.read();

      switch (blt)
      {

      case 'A':
        manualMode = false;
        break;
      case 'O':
        open_head();
        break;

      case 'C':
        close_head();
        break;

      case 'S':
        status();
        break;

      case 'M':
        slowMeasure();
        break;

      case 'D':
        normalMeasure();
        break;
      case 'F':
        fastMeasure();
        break;
      }
    }
  }
}

void printSerialFromLaser(String s)
{
  Serial.println("Laser: " + s);
}

void displayLcd(String string)
{
  lcd.clear();
  lcd.print(string);
}

void close_head()
{
  Serial.println("close");
  laserSerial.listen();
  laserSerial.println("C");
  delay(500);
  if (laserSerial.available())
  {
    message = laserSerial.readString();
    printSerialFromLaser(message);
    displayLcd(message);
  }
}

void open_head()
{
  Serial.println("open");
  laserSerial.listen();
  laserSerial.println("O");
  delay(500);
  if (laserSerial.available())
  {
    message = laserSerial.readString();
    printSerialFromLaser(message);
    displayLcd(message);
  }
}

void status()
{
  displayLcd("Starting");
  Serial.println("status");
  laserSerial.listen();
  laserSerial.println("S");
  delay(500);
  if (laserSerial.available())
  {
    message = laserSerial.readString();
  }
  else
  {
    message = "status: no response";
  }
  printSerialFromLaser(message);
  displayLcd(message);

  if (manualMode)
  {
    lcd.setCursor(0, 1);
    lcd.print("manual mode");
  }
}
// output sometimes contains an extra whitespace, so parse it defensively
String parseNumber(String input)
{
  int start = -1;
  int end;
  for (byte i = 0; i < input.length(); i++)
  {
    if (isDigit(input.charAt(i)) || input.charAt(i) == '.')
    {
      if (start == -1)
      {
        start = i;
      }
      end = i;
    }
    else if (start > -1)
    {
      break;
    }
  }

  if (start == -1)
  {
    return "0.00";
  }
  return input.substring(start, end + 1);
}

void measure(char string, int measureDelay)
{
  Serial.println("measure");
  laserSerial.listen();
  laserSerial.println(string); // send letter "F" to laser head
  delay(measureDelay);

  char output[100];
  int i = 0;
  while (laserSerial.available() > 0)
  {
    output[i] = laserSerial.read();
    i++;
  }

  if (i <= 1)
  {
    Serial.println("no response from laser");
    displayLcd("LASER NEODPOVEDEL");
    delay(500);
    return;
  }

  Serial.println("raw output");
  Serial.println(output);
  Serial.println("------");

  String str(output);

  int delim = str.indexOf("m,");
  if (delim <= 0)
  {
    Serial.println("Error");
    lcd.clear();
    lcd.print(str.substring(0, 16));
    lcd.setCursor(0, 1);
    lcd.print(str.substring(16));
    delay(500);
    return;
  }

  String distance = parseNumber(str.substring(0, delim));
  String quality = parseNumber(str.substring(delim + 2));

  Serial.println("parsed:");
  Serial.println("distance:" + distance);
  Serial.println("quality:" + quality);
  lastMeasure = distance.toDouble();
  double lastQuality = quality.toDouble();

  if (lastMeasure == 0.00)
  {

    //        const char *errShortDistance = "Err-Short distance";
    //     const char *errShortDistance = "prilis blizko";

    //     Serial.println(errShortDistance);
    //     displayLcd(errShortDistance);
    //     lcd.setCursor(0, 1);
    //    lcd.setCursor(0, 1);
    //     lcd.print(lastMeasure);
    //     lcd.print("cm, ");
    //     lcd.print(lastQuality, 0);

    Serial.println("Error");
    lcd.clear();
    lcd.print(str.substring(0, 16));
    lcd.setCursor(0, 1);
    lcd.print(str.substring(16));
    // delay(500);
    return;
  }
  Serial.println("converted:");
  Serial.println("lastMeasure:");
  Serial.println(lastMeasure);
  Serial.println("lastQuality:");
  Serial.println(lastQuality);

  // Serial.print("lastMeasure=");
  // Serial.print(lastMeasure, 3);
  // Serial.print("m,");
  // Serial.printTn("'" + String(lastQuality, 0) + "'");

  if (lastQuality > 1000 || lastQuality == 0.00)
  {
    //bugged piece of shit, with a bad quality, it returns 34m or 0.0034m instead of 0.34m

    const char *errBadSurface = "spatny povrch";
    //        const char *errBadSurface = "Err-Bad surface";

    Serial.println(errBadSurface);
    displayLcd(errBadSurface);
    lcd.setCursor(0, 1);
    lcd.print(lastMeasure);
    lcd.print("cm, ");
    lcd.print(lastQuality, 0);
    // delay(500);
  }
  else
  {
    lcd.clear();
    lcd.print((lastMeasure - tare) * 100, 1);
    lcd.print("cm, ");
    lcd.print(lastQuality, 0);
    if (tare != 0.0)
    {
      lcd.setCursor(0, 1);
      lcd.print("= ");
      lcd.print(lastMeasure * 100, 1);
      lcd.print("cm");
    }
  }
  Serial.println("end of measure");
  delay(0);
}

void interrupted()
{
  Serial.println("interrupt");
  if (buttonHold >= 2)
  {
    Serial.println("buttonHold>=2");
    return;
  }
  tare = lastMeasure;
}
