/* Requires espsoftwareserial.h onReceive functionality */

#include <Arduino.h>
#include <SoftwareSerial.h>
#include "Pms5003Async.h"

#define PMS_RX_PIN D4 // Rx from PMS (== PMS Tx)
#define PMS_TX_PIN D7 // Tx to PMS (== PMS Rx)

#ifdef ESP32
// TODO: TBC
HardwareSerial &pmsSerial(Serial2);
Pms5003Async<HardwareSerial> pms(pmsSerial);
#else
SoftwareSerial pmsSerial;
Pms5003Async<SoftwareSerial> pms(pmsSerial);
#endif

constexpr int tblSize = 30;
Pms5003::PMSDATA_t data_tbl[tblSize];
bool isPmsRunning = true;

void showData(Pms5003::PMSDATA_t obs)
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

void startPms()
{
  Serial.println("StartPms: Start wakeup PMS sensor");
  if (pms.set_sleep(false))
  {
    isPmsRunning = true;
  }
  Serial.println("StartPms: End wakeup PMS sensor");
}

void stopPms()
{
  Serial.println("StopPms: Start sleep PMS sensor");
  if (pms.set_sleep(true))
  {
    isPmsRunning = false;
  }
  Serial.println("StopPms: End sleep PMS sensor");
}

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

  Serial.println("Setup: PMS start/stop and reporting sample");

  // The sensor always wakes in active mode, but we'll command it just to be sure
  if (pms.get_data_reporting_mode() != Pms5003::REPORT_ACTIVE)
  {
    Serial.println("Setup: Turning on Pms5003::REPORT_ACTIVE reporting mode");
    if (!pms.set_data_reporting_mode(Pms5003::REPORT_ACTIVE))
    {
      Serial.println("Setup: Pms5003::set_data_reporting_mode(Pms5003::REPORT_ACTIVE) failed");
    }
  }

  delay(1000); // without this delay, stopPms is ignored on the first pass through the loop
}

void loop()
{
  // Per manufacturer specification, place the sensor to sleep to prolong service life.
  // Specifies a user-determined interval (here 60s down, plus 60s duty = 2m).
  // The manufacturer claims 30s are required for the sensor to stabilize
  // so omit the measurements obtained during the first 30s of each run.

  constexpr uint32_t down_s = 60;
  constexpr uint32_t duty_s = 60;
  pms.set_data_rampup(30);

  stopPms();
  Serial.print("stopped PMS sensor (is running = ");
  Serial.print(isPmsRunning);
  Serial.println(")");

  uint32_t deadline = millis() + down_s * 1000;
  while (static_cast<int32_t>(deadline - millis()) > 0)
  {
    delay(1000);
    Serial.println(static_cast<int32_t>(deadline - millis()) / 1000);
    pms.perform_work(); // SoftwareSerial 
  }

  startPms();
  Serial.print("started PMS sensor (is running = ");
  Serial.print(isPmsRunning);
  Serial.println(")");

  pms.on_query_data_auto_completed([](int n) {
    Serial.println("Begin Handling PMS query data");
    Pms5003::PMSDATA_t data;
    Serial.print("n = ");
    Serial.println(n);
    if (pms.average_data(n, data_tbl, data))
    {
      showData(data);
    }
    Serial.println("End Handling PMS query data");
  });

  if (!pms.query_data_auto_async(tblSize, data_tbl))
  {
    Serial.println("Measurement capture start failed");
  }

  deadline = millis() + duty_s * 1000;
  while (static_cast<int32_t>(deadline - millis()) > 0)
  {
    delay(1000);
    Serial.println(static_cast<int32_t>(deadline - millis()) / 1000);
    pms.perform_work(); // SoftwareSerial
  }
}
