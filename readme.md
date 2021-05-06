# ESP-Plantower-PMS

Arduino library for the Plantower PMS5003 particulate matter sensor 
attached to the ESP8266. Should also work for ESP32 boards but currently 
untested. The base Pms5003 class is generic and should also work for most 
AVR boards. The base Pms5003 class likely also works for other Plantower
PMS sensors as, for example, the PMS 5003 and 7003 share the same serial
specs and data format. Other sensors, including the PMS 1003 and 3003 do
not output the accumulated particle number concentration.

# Overview

The Plantower PMS5003 sensor uses laser scattering to infer the equivalent
particle diameter and the number of particles with different diameter
per unit volume consistent with MIE theory. That is to say, this sensor
**does not directly count particles or measure their mass concentration**
but estimates these based on scattering of the input light 
(wavelength ~0.65 um) collected ~90 degrees from the incident direction. 
The PMS5003 is similar to the initial PMS1003. Within the package, the 
CPU is a Cypress CY8C4245, a combined ARM Cortex-M0 running at 48Mhz 
with a dedicated ADC, used to sample the output from the diode.

Particulate Matter (PM) is typically a mixture of solid particles and
liquid droplets in the air.  PM is commonly classified by size into
coarse, fine, and ultrafine particles that correspond to particle
swith aerodynamic diameters of less than 10μm (PM10), 2.5μm (PM2.5),
and 0.1μm (nanoparticles), respectively.  Air samples typically
contain polydisperse aerosols, i.e. particulates of varying sizes at
different concentrations.

The PMS5003 sensor provides data in 6 size channels for accumulated
particle number concentration (>0.3, >0.5, >1.0, >2.5, >5.0, >10.0 μm)
as well as 3 channels for mass concentration (PM1, PM2.5, PM10). 
The unit of mass concentration is μg/m³. The 3 mass concentrations
are further reported as 'standard' and 'environment' by the
manufacturer. While the exact distinction isn't explicitly clear, 
the “standard” reading refers to calibration of the scattered light intensity
against industrial metal particles and is suitable for environments 
such as an industrial production workshop. The “atmospheric” reading refers 
to calibration of the scattered light intensity against pollutants commonly found
in the atmosphere, and suitable for use in an indoor environment. Both outputs 
give number of particles with different size per unit volume.

For a discussion on the calibration and performance of this sensor, see:

 Meilu He, Nueraili Kuerbanjiang & Suresh Dhaniyala (2020) Performance characteristics 
 of the low-cost Plantower PMS optical sensor, Aerosol Science and Technology, 
 54:2, 232-241, DOI: 10.1080/02786826.2019.1696015 

# Operation

There are two modes of sensor operation: passive and active. Default
mode is active after power up. In this mode sensor sends serial data
to the host automatically.

The active mode is divided into two sub-modes:
- stable mode: if the concentration change is small the sensor 
  reports with the interval of 2.3s.
- fast mode: if the change in particle counts is rapid, the
  sensor will report with the interval of 200~800ms.
  The higher of the concentration, the shorter of the
  interval.

The passive mode reports data only when requested.

## Notes on Use

1.  The PMS sensor ALWAYS wakes/boots into active-mode. It doesn't matter if you
    use PIN3/PIN6 to control the sensor or serial commands.

2.  It is unclear from the datasheet but the sensor responds to the sleep
    and wakeup commands in either active or passive mode.
      sleep:  `0x42 0x4D 0xE4 0x00 0x00 0x01 0x73`
      wakeup: `0x42 0x4D 0xE4 0x00 0x01 0x01 0x74`

3.  There is no way to query the sensor as to whether it is in active or
    passive mode. If the sensor is in active mode, it constantly dumps
    32 byte data frames/packets onto the serial line. In passive mode, the sensor
    is quiet and only produces data in response to the read command
    (`0x42 0x4D 0xE2 0x00 0x00 0x01 0x71`).

4.  After issuing a wakeup command, if left in active-mode, the sensor
    produces a number of data frames/packets with zeros for all data. The
    datasheet notes the sensor needs ~30s to equilibrate and produce
    reliable data. Frames/packets with non-zero data do arrive before 30s but
    should probably be discarded.

5.  It takes approximately 1300ms for the sensor to boot after it recieves
    a wakeup signal. All serial commands given before the sensor boots are
    lost, i.e. you must give the 'wake' command, wait for > 1300ms, and
    then give the passive-mode command if you want the sensor to be in
    passive mode. Alternatively, give the 'wake' command, wait for serial
    data to appear on the bus and then give the passive-mode command.

6.  The sensor produces a response packet for the following commands. 
    This includes the start and veryfy bytes shown in the datasheet. Recall that in
    active mode the sensor constantly sends 32-byte data packets.

```
            | Command                            | Expected Response
    --------+------------------------------------+-----------------------------------------
    active  | 0x42 0x4D 0xE1 0x00 0x01 0x01 0x71 | 0x42 0x4D 0x00 0x04 0xE1 0x01 0x01 0x75
    passive | 0x42 0x4D 0xE1 0x00 0x00 0x01 0x70 | 0x42 0x4D 0x00 0x04 0xE1 0x00 0x01 0x74
    sleep   | 0x42 0x4D 0xE4 0x00 0x00 0x01 0x73 | 0x42 0x4D 0x00 0x04 0xE4 0x00 0x01 0x77
    wakeup  | 0x42 0x4D 0xE4 0x00 0x01 0x01 0x74 | none
    read    | 0x42 0x4D 0xE2 0x00 0x00 0x01 0x71 | one 32-byte data packet
```

7.  The input (serial) buffer for an arduino, either SoftwareSerial or UART,
    is as small. For the PMSx0003 sensors, transfer is at 9600bps.
    Since there is a start bit, 8 data bits, and a stop bit (i.e. 9600 8N1)
    the PMS sends a byte of data every (1 / 9600) * 10 - seconds, or 1.04 ms.
    Therefore, a 64 byte buffer fills in about 66 ms, and further data will
    remove previously recieved data (FIFO).

## Serial Communication:
  - Default baud rate: 9600bps
  - Check-bit: None
  - Stop bit: 1 bit

Serial packets have a maximum length of 32 bytes with the following
defintion:

```
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
```

Where the checksum includes all frame/packet bytes (minus the checksum)

## Hardware Connections:

```
PIN1 VCC   Positive power 5V
PIN2 GND   Negative power
PIN3 SET   Optional - HW set TTL level of 3.3V high level for operation or
           low level for sleep mode. See note below.
PIN4 RX    Serial port receiving TTL level 3.3V
PIN5 TX    Serial port sending TTL level 3.3V
PIN6 RESET Optional - HW module reset signal TTL level@3.3V，low reset.
```