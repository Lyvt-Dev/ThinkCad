// Compile genuine libraries together, with both native and debug transports.
// No upstream source is patched to accommodate host-platform declarations.
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_BME280.h>
#include <Adafruit_MPU6050.h>
#include <MFRC522.h>
#include <DallasTemperature.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

static_assert(INPUT==0&&OUTPUT==1);
static_assert(MFRC522::STATUS_TIMEOUT==3);
static_assert(sizeof(MFRC522::PCD_Register)==1);
