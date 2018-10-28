#include <Arduino.h>
#include <SPI.h>
#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "Adafruit_BluefruitLE_UART.h"

#include "BluefruitConfig.h"

#define FACTORYRESET_ENABLE         0
#define MINIMUM_FIRMWARE_VERSION    "0.6.6"

static void error(void);

static Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);

void setup(void)
{
  /* LED for blinking when there's an error */
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(115200);
  Serial.println(F("Adafruit Bluefruit HID Keyboard Example"));
  Serial.println(F("---------------------------------------"));

  /* Initialise the module */
  Serial.print(F("Initialising the Bluefruit LE module: "));
  if (!ble.begin(VERBOSE_MODE))
  {
    error(F("Couldn't find Bluefruit, make sure it's in CoMmanD mode & check wiring?"));
  }
  Serial.println(F("OK!"));

  if (FACTORYRESET_ENABLE)
  {
    /* Perform a factory reset to make sure everything is in a known state */
    Serial.println(F("Performing a factory reset: "));
    if (!ble.factoryReset())
    {
      error(F("Couldn't factory reset"));
    }
  }

  /* Disable command echo from Bluefruit */
  ble.echo(false);

  /* Print Bluefruit information */
  Serial.println("Requesting Bluefruit info:");
  ble.info();

  /* Change the device name to make it easier to find */
  Serial.println(F("Setting device name to 'Bluefruit Keyboard R': "));
  if (!ble.sendCommandCheckOK(F("AT+GAPDEVNAME=Bluefruit Keyboard R")))
  {
    error(F("Could not set device name?"));
  }

  /* Enable HID Service */
  Serial.println(F("Enable HID Service (including Keyboard): "));
  if (ble.isVersionAtLeast(MINIMUM_FIRMWARE_VERSION))
  {
    if (!ble.sendCommandCheckOK(F("AT+BleHIDEn=On")))
    {
      error(F("Could not enable Keyboard"));
    }
  }
  else
  {
    if (!ble.sendCommandCheckOK(F("AT+BleKeyboardEn=On")))
    {
      error(F("Could not enable Keyboard"));
    }
  }

  /* Add or remove service requires a reset */
  Serial.println(F("Performing a SW reset (service changes require a reset): "));
  if (!ble.reset())
  {
    error(F("Couldn't reset??"));
  }

  Serial.println(F("Ready to go!"));
  Serial.println();
}

void loop(void)
{
  ble.sendCommandCheckOK("AT+BleKeyboard=G\r");
  delay(250);
}

static void error(const __FlashStringHelper*err)
{
  while (1)
  {
    if (!Serial)
    {
      Serial.begin(115200);
    }
    Serial.println(err);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(250);
    digitalWrite(LED_BUILTIN, LOW);
    delay(250);
  }
}
