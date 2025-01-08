/*
    This example demonstrates the use of multiple USB CDC/ACM "Virtual
    Serial" ports

    Written by Bill Westfield (aka WestfW), June 2021.
    Copyright 2021 by Bill Westfield
    MIT license, check LICENSE for more information
*/

/*
 * This does not appaer to work for me on the rpipico2 :(
*/

/* The example creates two virtual serial ports. Text entered on
 * any of the ports will be echoed to the all ports with
 *  - all lower case in port0 (Serial)
 *  - all upper case in port1
 *
 * Requirement:
 *  The max number of CDC ports (CFG_TUD_CDC) has to be changed to at least 2.
 *  Config file is located in Adafruit_TinyUSB_Arduino/src/arduino/ports/{platform}/tusb_config_{platform}.h
 *  where platform is one of: nrf, rp2040, samd
 *
 *  NOTE: Currnetly multiple CDCs on ESP32-Sx is not yet supported.
 *  An PR to update core/esp32/USBCDC and/or pre-built libusb are needed.
 *  We would implement this later when we could.
 */
//#define CFG_TUD_CDC 4
#include <Adafruit_TinyUSB.h>

#define LED LED_BUILTIN

// Create 2nd instance of CDC Ports.
#ifdef ARDUINO_ARCH_ESP32
  #error "Currently multiple CDCs on ESP32-Sx is not yet supported. An PR to update core/esp32/USBCDC and/or pre-built libusb are needed."
  // for ESP32, we need to specify instance number when declaring object
  Adafruit_USBD_CDC USBSer1(1);
#else
  Adafruit_USBD_CDC USBSer1;
  Adafruit_USBD_CDC USBSer2;
#endif


void printAll(int ch);
boolean delay_without_delaying(unsigned long time);


#define BUF_LENGTH 128  /* Buffer for the incoming command. */

static bool do_echo = true;
float tempC;
void print_prompt(void)
{
  tempC = analogReadTemp(); // Get internal temperature
  Serial.print(tempC);
  Serial.print(F("ºC > "));
}

/* Execute a complete command. */
static void exec(char *cmdline, int length)
{
    char *command = strsep(&cmdline, " ");

    if (strcmp_P(command, PSTR("help")) == 0) {
        Serial.println(F(
            "mode <pin> <mode>: pinMode()\r\n"
            "read <pin>: digitalRead()\r\n"
            "aread <pin>: analogRead()\r\n"
            "write <pin> <value>: digitalWrite()\r\n"
            "awrite <pin> <value>: analogWrite()\r\n"
            "msg <message>: Write a message to USBSer2\r\n"
            "echo <value>: set echo off (0) or on (1)"));
    } else if (strcmp_P(command, PSTR("mode")) == 0) {
        int pin = atoi(strsep(&cmdline, " "));
        int mode = atoi(cmdline);
        pinMode(pin, mode);
    } else if (strcmp_P(command, PSTR("read")) == 0) {
        int pin = atoi(cmdline);
        Serial.println(digitalRead(pin));
    } else if (strcmp_P(command, PSTR("aread")) == 0) {
        int pin = atoi(cmdline);
        Serial.println(analogRead(pin));
    } else if (strcmp_P(command, PSTR("write")) == 0) {
        int pin = atoi(strsep(&cmdline, " "));
        int value = atoi(cmdline);
        digitalWrite(pin, value);
    } else if (strcmp_P(command, PSTR("awrite")) == 0) {
        int pin = atoi(strsep(&cmdline, " "));
        int value = atoi(cmdline);
        analogWrite(pin, value);
    } else if (strcmp_P(command, PSTR("msg")) == 0) {
        USBSer2.println(cmdline);
    } else if (strcmp_P(command, PSTR("echo")) == 0) {
        do_echo = atoi(cmdline);
    } else {
        Serial.print(F("Error: Unknown command: "));
        Serial.println(command);
    }
    print_prompt();
}

void setup() {
  pinMode(LED, OUTPUT);

  Serial.begin(115200);

  // check to see if multiple CDCs are enabled
  if ( CFG_TUD_CDC < 2 ) {
    digitalWrite(LED, HIGH); // LED on for error indicator

    while(1) {
      Serial.printf("CFG_TUD_CDC must be at least 2, current value is %u\n", CFG_TUD_CDC);
      Serial.println("  Config file is located in Adafruit_TinyUSB_Arduino/src/arduino/ports/{platform}/tusb_config_{platform}.h");
      Serial.println("  where platform is one of: nrf, rp2040, samd");
      delay(1000);
    }
  }

  // initialize 2nd CDC interface
  USBSer1.begin(115200);
  // initialize 3rd CDC interface
  USBSer2.begin(115200);


  // If already enumerated, additional class driverr begin() e.g msc, hid, midi won't take effect until re-enumeration
  if (TinyUSBDevice.mounted()) {
    TinyUSBDevice.detach();
    delay(10);
    TinyUSBDevice.attach();
  }

  while (!Serial || !USBSer1) {
    if (Serial) {
      Serial.println("Serial: Waiting for other USB ports");
    }

    if (USBSer1) {
      USBSer1.println("USBSer1: Waiting for other USB ports");
    }

    delay(1000);
  }

  Serial.print("You are port 0\n\r\n0> ");
  USBSer1.print("You are port 1\n\r\n1> ");
}

int LEDstate = 0;

void loop() {
  /*
  int ch;

  ch = Serial.read();
  if (ch > 0) {
    printAll(ch);
  }

  ch = USBSer1.read();
  if (ch > 0) {
    printAll(ch);
  }
*/
  if (delay_without_delaying(500)) {
    LEDstate = !LEDstate;
    digitalWrite(LED, LEDstate);
  }
}


void setup1() {}
void loop1() {

    /* Process incoming commands. */
    while (Serial.available()) {
        static char buffer[BUF_LENGTH];
        static int length = 0;

        int data = Serial.read();
        if (data == '\b' || data == '\177') {  // BS and DEL
            if (length) {
                length--;
                if (do_echo) Serial.write("\b \b");
            }
        }
        else if (data == '\r') {
            if (do_echo) Serial.write("\r\n");    // output CRLF
            buffer[length] = '\0';
            if (length) {
              exec(buffer, length);
            } else {
              print_prompt();
            }
            length = 0;
        }
        else if (length < BUF_LENGTH - 1) {
            buffer[length++] = data;
            if (do_echo) Serial.write(data);
        }
    }

    /* Whatever else needs to be done... */
}


// print to all CDC ports
void printAll(int ch) {
  // always lower case
  Serial.write(tolower(ch));

  // always upper case
  USBSer1.write(toupper(ch));
}

// Helper: non-blocking "delay" alternative.
boolean delay_without_delaying(unsigned long time) {
  // return false if we're still "delaying", true if time ms has passed.
  // this should look a lot like "blink without delay"
  static unsigned long previousmillis = 0;
  unsigned long currentmillis = millis();
  if (currentmillis - previousmillis >= time) {
    previousmillis = currentmillis;
    return true;
  }
  return false;
}
