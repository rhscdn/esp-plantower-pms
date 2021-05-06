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
  delay(2000);
  Serial.begin(115200);
  delay(1000);

#ifdef ESP32
  pmsSerial.begin(9600, SERIAL_8N1, SDS_PIN_RX, SDS_PIN_TX);
  delay(100);
#else
  pmsSerial.begin(Pms5003::BAUD_RATE, SWSERIAL_8N1, PMS_RX_PIN, PMS_TX_PIN, false, 192);
#endif

  Serial.print("Set Passive: ");
  if (pms.set_data_reporting_mode(Pms5003::REPORT_PASSIVE) == true)
    Serial.println("ok");
  else
    Serial.println("failed");

  delay(10000);
}

void loop()
{

  if (pms.query_data(obs,10))
  {
    Serial.println("-----------------------------------------------");
    Serial.println("Concentration Units (standard)");
    Serial.print("PM 1.0: ");
    Serial.print(obs.pm10_std);
    Serial.print("\tPM 2.5: ");
    Serial.print(obs.pm25_std);
    Serial.print("\tPM 10: ");
    Serial.println(obs.pm100_std);
    Serial.println("-----------------------------------------------");
    Serial.println("Concentration Units (environmental)");
    Serial.print("PM 1.0: ");
    Serial.print(obs.pm10_env);
    Serial.print("\tPM 2.5: ");
    Serial.print(obs.pm25_env);
    Serial.print("\tPM 10: ");
    Serial.println(obs.pm100_env);
    Serial.println("-----------------------------------------------");
    Serial.println("Particles (> X um) / 0.1L air:");
    Serial.print("> 0.3um: ");
    Serial.print(obs.prt_03um);
    Serial.print("\t> 0.5um: ");
    Serial.print(obs.prt_05um);
    Serial.print("\t> 1.0um: ");
    Serial.println(obs.prt_10um);
    Serial.print("> 2.5um: ");
    Serial.print(obs.prt_25um);
    Serial.print("\t> 5.0um: ");
    Serial.print(obs.prt_50um);
    Serial.print("\t> 10.0 um: ");
    Serial.println(obs.prt_100um);
    Serial.println("-----------------------------------------------");
  }
  else
  {
    Serial.println("No Data");
  }

  delay(30000);

}
