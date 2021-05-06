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
  Serial.print("Set Active: ");
  if (pms.set_data_reporting_mode(Pms5003::REPORT_ACTIVE) == true)
    Serial.println("ok");
  else
    Serial.println("failed");
}

void loop()
{

  if (pms.query_data_auto(obs))
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

  delay(2000);

}

/*
NOTES:
^^^^^^
The PMS5003 sensor outputs the number of particles with different
size per unit volume, the unit volume of particle number is 0.1L
and the unit of mass concentration is μg/m³. There are two options
for digital output: passive and active. Default mode is active
after power up. In this mode sensor sends serial data to the host
automatically.

The active mode is divided into two sub-modes:
   stable mode: if the concentration change is small the sensor
                reports with the interval of 2.3s.
   fast mode:   If the change in particle counts is rapid, the
                sensor will report with the interval of 200~800ms.
                The higher of the concentration, the shorter of the
                interval.

The passive mode reports data only when requested.

Serial communication:
  Default baud rate: 9600bps
  Check-bit: None
  Stop bit: 1 bit

Serial packets have a maximum length of 32 bytes with the following
defintion:

first byte        0 - 0x42
second byte      01 - 0x4d
payload length   02 - high byte
payload length   03 - low byte
data 1           04 - high byte
data 1           05 - low byte
data 2           06 - high byte
data 2           07 - low byte
...                ...
data 13          28 - high byte
data 13          29 - low byte
checksum         30 - high byte
checksum         31 - low byte

Where the checksum includes all packet bytes (minus the checksum)

Connections:
PIN1 VCC   Positive power 5V
PIN2 GND   Negative power
PIN3 SET   Optional - HW set TTL level of 3.3V high level for operation or
           low level for sleep mode. See note below.
PIN4 RX    Serial port receiving TTL level 3.3V
PIN5 TX    Serial port sending TTL level 3.3V
PIN6 RESET Optional - HW module reset signal TTL level@3.3V，low reset.
           See note below.

Notes/Observations:
1)  The PMS sensor ALWAYS wakes/boots into active-mode. It doesn't matter if you
    use PIN3/PIN6 to control the sensor or serial commands.

2)  It is unclear from the datasheet but the sensor responds to the sleep
    and wakeup commands in either active or passive mode.
      sleep:  0x42 0x4D 0xE4 0x00 0x00 0x01 0x73
      wakeup: 0x42 0x4D 0xE4 0x00 0x01 0x01 0x74

3)  There is no way to query the sensor as to whether it is in active or
    passive mode. If the sensor is in active mode, it constantly dumps
    32 byte data packets onto the serial line. In passive mode, the sensor
    is quiet and only produces data in response to the read command
    (0x42 0x4D 0xE2 0x00 0x00 0x01 0x71).

4)  After issuing a wakeup command, if left in active-mode, the sensor
    produces a number of data packets with zeros for all data. The
    datasheet notes the sensor needs ~30s to equilibrate and produce
    reliable data. Packets with non-zero data do arrive before 30s but
    should probably be discarded.

5)  It takes approximately 1300ms for the sensor to boot after it recieves
    a wakeup signal. All serial commands given before the sensor boots are
    lost, i.e. you must give the 'wake' command, wait for > 1300ms, and
    then give the passive-mode command if you want the sensor to be in
    passive mode.

6)  The sensor produces a response packet for the following commands:

            | Command                            | Expected Response
    --------+------------------------------------+-----------------------------------------
    active  | 0x42 0x4D 0xE1 0x00 0x01 0x01 0x71 | 0x42 0x4D 0x00 0x04 0xE1 0x01 0x01 0x75
    passive | 0x42 0x4D 0xE1 0x00 0x00 0x01 0x70 | 0x42 0x4D 0x00 0x04 0xE1 0x00 0x01 0x74
    sleep   | 0x42 0x4D 0xE4 0x00 0x00 0x01 0x73 | 0x42 0x4D 0x00 0x04 0xE4 0x00 0x01 0x77
    wakeup  | 0x42 0x4D 0xE4 0x00 0x01 0x01 0x74 | none
    read    | 0x42 0x4D 0xE2 0x00 0x00 0x01 0x71 | one 32-byte data packet

    The above includes the start and veryfy bytes shown in the datasheet. Recall that in
    active mode the sensor constantly sends 32-byte data packets.

6b) How long does it take for response packets to arrive?
    TBC

7)  The input (serial) buffer for an arduino, either SoftwareSerial or UART,
    is as small. For the PMSx0003 sensors, transfer is at 9600bps.
    Since there is a start bit, 8 data bits, and a stop bit (i.e. 9600 8N1)
    the PMS sends a byte of data every (1 / 9600) * 10 - seconds, or 1.04 ms.
    Therefore, a 64 byte buffer fills in about 66 ms, and further data will
    remove previously recieved data (FIFO).
*/

//  static const uint8_t msgLen = 7;
//     const uint8_t act[msgLen] = {0x42, 0x4D, 0xE1, 0x00, 0x01, 0x01, 0x71}; // set active mode
//     const uint8_t slp[msgLen] = {0x42, 0x4D, 0xE4, 0x00, 0x00, 0x01, 0x73}; // sleep
//     const uint8_t wak[msgLen] = {0x42, 0x4D, 0xE4, 0x00, 0x01, 0x01, 0x74}; // wake
//     const uint8_t cfg[msgLen] = {0x42, 0x4D, 0xE1, 0x00, 0x00, 0x01, 0x70}; // set passive mode
//     const uint8_t trg[msgLen] = {0x42, 0x4D, 0xE2, 0x00, 0x00, 0x01, 0x71}; // passive mode read

// bool setMode(MODE_ACTIVE/MODE_PASSIVE)
// bool setSleep
// bool setWake
// bool passiveRead
//
// Because the sensor cannot be queried, we could create
// a 'state' that holds info about the sensor state.
// sensor.
//
// This would allow some get routines:
//     pms::_mode
//     pms::_sleep
//   bool getMode returns
//   bool getSleep returns
//
// bool setRampup - sets the number of seconds we should discard
//      pms::_rampup_s
//
//    bool set_data_rampup(int secs) {
//        rampup_s = secs;
//        return true;
//    }
//    bool get_data_rampup(int& secs) {
//        secs = rampup_s;
//        return true;
//    }
//
// bool query_data(pmsData& d)      (passive)
// bool query_data_auto(pmsData& d) (active)
//
// bool timeout
// bool crc_ok()
//
//
//
//
// Because the sensor cannot be queried, we could create
// a 'state' that holds info about the sensor state.
// sensor
//    -mode: active/passive
//    -sleeping: true/fase
//
// This would allow some get routines:
//   bool getMode returns ACTIVE/PASSIVE
//   bool getSleep returns  true/false
