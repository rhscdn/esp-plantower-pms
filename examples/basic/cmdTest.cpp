#include <Arduino.h>
#include <SoftwareSerial.h>
#include "Pms5003.h"

#define PMS_RX_PIN D4 // Rx from PMS (== PMS Tx)
#define PMS_TX_PIN D7 // Tx to PMS (== PMS Rx)

#ifdef ESP32
// TODO: TBC
HardwareSerial &pmsSerial(Serial2);
Pms5003 pms(pmsSerial);
#else
SoftwareSerial pmsSerial;
Pms5003 pms(pmsSerial);
#endif

Pms5003::PMSDATA_t obs;

void setup()
{
  delay(3000);
  Serial.begin(115200);
  delay(1000);

#ifdef ESP32
  pmsSerial.begin(9600, SERIAL_8N1, SDS_PIN_RX, SDS_PIN_TX);
  delay(100);
#else
  pmsSerial.begin(Pms5003::BAUD_RATE, SWSERIAL_8N1, PMS_RX_PIN, PMS_TX_PIN, false, 192);
#endif

}

void loop()
{
  Serial.print("Set Active: ");
  if (pms.set_data_reporting_mode(Pms5003::REPORT_ACTIVE) == true)
    Serial.println("ok");
  else
    Serial.println("failed");

  delay(20000);

  Serial.print("Set Sleep: ");
  if (pms.set_sleep(true) == true)
    Serial.println("ok");
  else
    Serial.println("failed");

  delay(20000);

  Serial.print("Set Wake: ");
  if (pms.set_sleep(false) == true)
    Serial.println("ok");
  else
    Serial.println("failed");

  delay(20000);

  Serial.print("Set Passive: ");
  if (pms.set_data_reporting_mode(Pms5003::REPORT_PASSIVE) == true)
    Serial.println("ok");
  else
    Serial.println("failed");

  delay(20000);

  Serial.print("Set Sleep: ");
  if (pms.set_sleep(true) == true)
    Serial.println("ok");
  else
    Serial.println("failed");

  delay(20000);

  Serial.print("Set Wake: ");
  if (pms.set_sleep(false) == true)
    Serial.println("ok");
  else
    Serial.println("failed");

  delay(20000);
}
