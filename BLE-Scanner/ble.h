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

#ifndef __BLE_H__
#define __BLE_H__ 1

#include "config.h"
#include "macaddr.h"

/*
   available in the Library Manger
   https://github.com/nkolban/ESP32_BLE_Arduino
*/
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

/*
    Bluetooth LE settings (in seconds)
*/
#define BLE_SCAN_TIME_MIN     5
#define BLE_SCAN_TIME_MAX     100
#define BLE_PAUSE_TIME_MIN    0
#define BLE_PAUSE_TIME_MAX    (24 * 60 * 60)

/*
   how mandy device should be keep in our list
*/
#define BLE_DEVICE_LIST_MAX_LENGTH    1000

/*
   how many chars should be store from the name of a device

*/
#define BLE_DEVICE_NAME_LENGTH        20

/*
   struct to hold a found BLE device
*/
typedef struct _ble_device {
  byte mac[MAC_ADDR_LEN];
  int rssi;
  char name[BLE_DEVICE_NAME_LENGTH + 1];
  const char *vendor;
  time_t last_seen;
  struct _ble_device *prev;
  struct _ble_device *next;
} BLE_DEVICE;

/*
   setup
*/
void BleSetup(void);

/*
   initiate a BLE scan
*/
void BleStartScan(void);

/*
   return the last BLE scan list as HTML
*/
String BleScanListHTML(void);

#endif

/**/
