#include <Arduino.h>
#include <SPI.h>
#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "Adafruit_BluefruitLE_UART.h"
#include "Adafruit_FreeTouch.h"

#include "BluefruitConfig.h"

#define FACTORYRESET_ENABLE         0
#define MINIMUM_FIRMWARE_VERSION    "0.6.6"
#define MODE_LED_BEHAVIOUR          "DISABLE"
#define NUM_BUTTONS                 5
#define OVERSAMPLE_AMOUNT           OVERSAMPLE_1
#define SERIES_RESISTOR             RESISTOR_50K
#define FREQ_MODE                   FREQ_MODE_NONE
#define DEBOUNCE_DELAY              30

typedef struct
{
  Adafruit_FreeTouch qt;
  uint64_t lastDebounceTime;
  int thresh;
  const __FlashStringHelper* keycode;
  uint8_t pressed;
  uint8_t prevState;
  
} gameboyButton_t;

static gameboyButton_t buttons[NUM_BUTTONS];
static Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);

static void readButtons(void);
static void sendButtons(void);
static void error(void);

void setup(void)
{
  int i;

//  while (!Serial);
  
  /* LED for blinking when there's an error */
  pinMode(LED_BUILTIN, OUTPUT);

  /* Button setup */
  memset(buttons, 0x00, sizeof(buttons));

  /* L (L) */
  buttons[0].qt = Adafruit_FreeTouch(A1, OVERSAMPLE_AMOUNT, SERIES_RESISTOR, FREQ_MODE);
  buttons[0].thresh = 500;
  buttons[0].keycode = F("0F");

  /* Left (A) */
  buttons[1].qt = Adafruit_FreeTouch(A2, OVERSAMPLE_AMOUNT, SERIES_RESISTOR, FREQ_MODE);
  buttons[1].thresh = 500;
  buttons[1].keycode = F("04");

  /* Up (W) */
  buttons[2].qt = Adafruit_FreeTouch(A3, OVERSAMPLE_AMOUNT, SERIES_RESISTOR, FREQ_MODE);
  buttons[2].thresh = 500;
  buttons[2].keycode = F("1A");

  /* Right (D) */
  buttons[3].qt = Adafruit_FreeTouch(A4, OVERSAMPLE_AMOUNT, SERIES_RESISTOR, FREQ_MODE);
  buttons[3].thresh = 500;
  buttons[3].keycode = F("07");

  /* Down (S) */
  buttons[4].qt = Adafruit_FreeTouch(A5, OVERSAMPLE_AMOUNT, SERIES_RESISTOR, FREQ_MODE);
  buttons[4].thresh = 500;
  buttons[4].keycode = F("16");

  for (i = 0; i < NUM_BUTTONS; ++i)
  {
    if (!buttons[i].qt.begin())
      error(F("Could not initialize button"));
  }

  /* Serial setup */
  Serial.begin(115200);
  Serial.println(F("Adafruit Bluefruit HID Keyboard Example"));
  Serial.println(F("---------------------------------------"));

  /* Initialise the BLE module */
  Serial.print(F("Initialising the Bluefruit LE module: "));
  if (!ble.begin(VERBOSE_MODE))
    error(F("Couldn't find Bluefruit, make sure it's in CoMmanD mode & check wiring?"));
  Serial.println(F("OK!"));

  if (FACTORYRESET_ENABLE)
  {
    /* Perform a factory reset to make sure everything is in a known state */
    Serial.println(F("Performing a factory reset: "));
    if (!ble.factoryReset())
      error(F("Couldn't factory reset"));
  }

  /* Disable command echo from Bluefruit */
  ble.echo(false);

  /* Print Bluefruit information */
  Serial.println("Requesting Bluefruit info:");
  ble.info();

  /* Change the device name to make it easier to find */
  Serial.println(F("Setting device name to 'Bluefruit Keyboard L': "));
  if (!ble.sendCommandCheckOK(F("AT+GAPDEVNAME=Bluefruit Keyboard L")))
    error(F("Could not set device name?"));

  /* Enable HID Service */
  Serial.println(F("Enable HID Service (including Keyboard): "));
  if (ble.isVersionAtLeast(MINIMUM_FIRMWARE_VERSION))
  {
    if (!ble.sendCommandCheckOK(F("AT+BleHIDEn=On")))
      error(F("Could not enable Keyboard"));
  }
  else
  {
    if (!ble.sendCommandCheckOK(F("AT+BleKeyboardEn=On")))
      error(F("Could not enable Keyboard"));
  }

  /* Add or remove service requires a reset */
  Serial.println(F("Performing a SW reset (service changes require a reset): "));
  if (!ble.reset())
    error(F("Couldn't reset??"));

  ble.sendCommandCheckOK("AT+HWModeLED=" MODE_LED_BEHAVIOUR);

  Serial.println(F("Ready to go!"));
  Serial.println();
}

void loop(void)
{
  readButtons();
  sendButtons();
}

static void readButtons(void)
{
  int i, result;
  uint8_t newState;

  for (i = 0; i < NUM_BUTTONS; ++i)
  {
    newState = buttons[i].prevState;
    result = buttons[i].qt.measure();
    if (result > buttons[i].thresh)
      newState = 1;
    else if (result > 0)
      newState = 0;

    if (newState != buttons[i].prevState)
      buttons[i].lastDebounceTime = millis();
    if (millis() - buttons[i].lastDebounceTime > DEBOUNCE_DELAY)
      buttons[i].pressed = newState;

    buttons[i].prevState = newState;
  }
}

static void sendButtons(void)
{
  int i;
  
  ble.print(F("AT+BleKeyboardCode=00-00"));
//  Serial.print(F("AT+BleKeyboardCode=00-00"));
  for (i = 0; i < NUM_BUTTONS; ++i)
  {
    if (buttons[i].pressed)
    {
      ble.print(F("-"));
      ble.print(buttons[i].keycode);

//      Serial.print(F("-"));
//      Serial.print(buttons[i].keycode);
    }
  }
  ble.println();
//  Serial.println();
  
  ble.waitForOK();
//  Serial.println(F("OK!"));
}

static void error(const __FlashStringHelper*err)
{
  while (1)
  {
    if (!Serial)
      Serial.begin(115200);
    Serial.println(err);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(250);
    digitalWrite(LED_BUILTIN, LOW);
    delay(250);
  }
}
