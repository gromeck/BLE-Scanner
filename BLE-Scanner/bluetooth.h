/*
  BLE-Scanner

  (c) 2020 Christian.Lorenz@gromeck.de

  module to handle the BLE scan


  This file is part of BLE-Scanner.

  BLE-Scanner is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  BLE-Scanner is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with BLE-Scanner.  If not, see <https://www.gnu.org/licenses/>.

*/

#ifndef __BLUETOOTH_H__
#define __BLUETOOTH_H__ 1
#include <NimBLEDevice.h>
#include "config.h"
#include "ble-manufacturer.h"

/*
    Bluetooth settings
*/
#define BLUETOOTH_SCAN_TIME_MIN               3             // seconds
#define BLUETOOTH_SCAN_TIME_MAX               (3 * 60)
#define BLUETOOTH_PAUSE_TIME_MIN              10            // seconds
#define BLUETOOTH_PAUSE_TIME_MAX              (24 * 60 * 60)
#define BLUETOOTH_ACTIVESCAN_TIMEOUT_MIN      60            // seconds
#define BLUETOOTH_ACTIVESCAN_TIMEOUT_MAX      (24 * 60 * 60)
#define BLUETOOTH_ABSENCE_CYCLES_MIN          1             // cycles
#define BLUETOOTH_ABSENCE_CYCLES_MAX          10
#define BLUETOOTH_BATTCHECK_TIMEOUT_MIN       60            // seconds
#define BLUETOOTH_BATTCHECK_TIMEOUT_MAX       (24 * 60 * 60)


/*
    service & characteristic UUIDs for the battery
*/
#define BLEBatteryService           BLEUUID((uint16_t)0x180F)
#define BLEBatteryCharacteristics   BLEUUID((uint16_t)0x2A19)


/*
   setup the bluetooth stuff
*/
void BluetoothSetup(void);

/*
   do the cyclic update
*/
void BluetoothUpdate(void);

/*
   init/exit the scanning
*/
bool BluetoothScanStart(void);
bool BluetoothScanStop(void);

/*
   get battery level
*/
bool BluetoothBatteryCheck(BLEAddress device, uint8_t *battery_level);

#endif

/**/
