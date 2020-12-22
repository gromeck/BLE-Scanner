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
#define BLE_SCAN_TIME     10
#define BLE_PAUSE_TIME    15

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
