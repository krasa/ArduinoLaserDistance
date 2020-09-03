//based on https://github.com/BaraVu/Arduino_laser_distance

#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>

LiquidCrystal_I2C lcd(0x27, 16, 2); // set the LCD address to 0x27 for a 16 chars and 2 line display

//(Rx,Tx)
SoftwareSerial laserSerial(5, 4);

const byte buttonPin = 3;
boolean manualMode = false;
volatile double lastMeasure = 0.0;
volatile double tare = 0.0;

String message;
char Mereni[100];

void measure(char string, int measureDelay);

void status();

void open_head();

void close_head();

void printSerialFromLaser(String s);

void displayLcd(String string);

void interrupted();

void manualProcessing();

void setup() {
    pinMode(buttonPin, INPUT);
    attachInterrupt(digitalPinToInterrupt(buttonPin), interrupted, RISING);

    laserSerial.begin(19200);
    Serial.begin(9600);

    lcd.init();
    lcd.backlight();

    if (digitalRead(buttonPin) == HIGH) {
        manualMode = true;
    }

    status();
    delay(1000);
}

volatile int buttonHold = 0;

void loop() {
    if (manualMode) {
        manualProcessing();
    } else {
        measure('F', 500);
        delay(500);

        if (digitalRead(buttonPin) == HIGH) {
            buttonHold++;
        } else {
            buttonHold = 0;
        }
        if (buttonHold >= 2) {
            tare = 0.0;
        }

        if (Serial.available()) {
            manualMode = true;
            displayLcd("manualni mod");
        }
    }
}

void manualProcessing() {
    if (Serial.available()) {
        // wait a bit for the entire message to arrive
        delay(100);

        while (Serial.available() > 0) {
            char blt = Serial.read();

            switch (blt) {

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
                    Serial.println("slow measure");
                    measure('M', 4000);
                    break;

                case 'D':
                    Serial.println("measure");
                    measure('D', 1000);
                    break;

                case 'F':
                    Serial.println("fast measure");
                    measure('F', 500);
                    break;
            }
        }
    }
}

void printSerialFromLaser(String s) {
    Serial.println("Laser: " + s);
}

void displayLcd(String string) {
    lcd.clear();
    lcd.print(string);
}

void close_head() {
    Serial.println("close");
    laserSerial.listen();
    laserSerial.println("C");
    delay(500);
    if (laserSerial.available()) {
        message = laserSerial.readString();
        printSerialFromLaser(message);
        displayLcd(message);
    }
}

void open_head() {
    Serial.println("open");
    laserSerial.listen();
    laserSerial.println("O");
    delay(500);
    if (laserSerial.available()) {
        message = laserSerial.readString();
        printSerialFromLaser(message);
        displayLcd(message);
    }
}

void status() {
    Serial.println("status");
    laserSerial.listen();
    laserSerial.println("S");
    delay(500);
    if (laserSerial.available()) {
        message = laserSerial.readString();
        printSerialFromLaser(message);
        displayLcd(message);
    }
}

void measure(char string, int measureDelay) {
    Serial.println("measure");
    laserSerial.listen();
    laserSerial.println(string); // send letter "F" to laser head
    delay(measureDelay);

    int i = 0;
    while (laserSerial.available() > 0) {
        Mereni[i] = laserSerial.read();
        i++;
    }

    if (i <= 1) {
        Serial.println("no response from laser");
        displayLcd("LASER NEODPOVEDEL");
        return;
    }

    //TODO error handling ':Er99!'
    char *strings[4];
    char *ptr = NULL;
    byte index = 0;
    ptr = strtok(Mereni, ":m,"); //divide message to strings
    strings[2] = NULL;           //set quality to null
    while (ptr != NULL) {
        strings[index] = ptr;
        index++;
        ptr = strtok(NULL, ":m, "); // takes a list of delimiters
        //  Serial.print(index);
        // Serial.println(strings[index]);
    }

    //strings[1] = "45.256"; //test
    //strings[2] = "0076"; //test

    lastMeasure = atof(strings[1]);
    double lastQuality = atof(strings[2]);

    Serial.print(lastMeasure, 3);
    Serial.print("m,");
    Serial.println("'" + String(lastQuality, 0) + "'");

    if (lastMeasure == 0.00) {
        //        const char *errShortDistance = "Err-Short distance";
        const char *errShortDistance = "prilis blizko";

        Serial.println(errShortDistance);
        displayLcd(errShortDistance);
    } else if (lastQuality > 1000 || lastQuality == 0.00) {
        const char *errBadSurface = "spatny povrch,uhel,daleko";
        //        const char *errBadSurface = "Err-Bad surface";

        Serial.println(errBadSurface);
        displayLcd(errBadSurface);
    } else {
        lcd.clear();
        lcd.print((lastMeasure - tare) * 100, 1);
        lcd.print("cm, ");
        lcd.print(lastQuality, 0);
        if (tare != 0.0) {
            lcd.setCursor(0, 1);
            lcd.print("= ");
            lcd.print(lastMeasure * 100, 1);
            lcd.print("cm");
        }
    }
    Serial.println("end of measure");
    delay(500);
}

void interrupted() {
    Serial.println("interrupt");
    if (buttonHold >= 2) {
        Serial.println("buttonHold>=2");
        return;
    }
    tare = lastMeasure;
}
