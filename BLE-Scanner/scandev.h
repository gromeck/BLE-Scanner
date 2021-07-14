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

#ifndef __SCANDEV_H__
#define __SCANDEV_H__ 1

#include "config.h"
#include "bluetooth.h"
#include "ble-manufacturer.h"

/*
   how mandy device should be keep in our list
*/
#define SCANDEV_LIST_MAX_LENGTH    1000

/*
   how many chars should be store from the name of a device

*/
#define SCANDEV_NAME_LENGTH        20


/*
   struct to hold a found BLE device
*/
typedef struct _scandev_device {
  /*
     identification of the device
  */
  BLEAddress addr;
  char name[SCANDEV_NAME_LENGTH + 1];

  /*
     manufacturer
  */
  uint16_t manufacturer_id;
  const char *manufacturer;

  /*
     battery
  */
  bool has_battery;
  uint8_t battery_level;
  time_t last_battcheck;

  /*
     state
  */
  time_t last_seen;
  bool present;
  int rssi;

  /*
     MQTT publishing
  */
  bool publish_last_seen;
  bool publish_name;
  bool publish_manufacturer;
  bool publish_battery;
  bool publish_rssi;
  bool publish_presence;
  bool publish;
  time_t last_published;

  /*
     book-keeping
  */
  struct _scandev_device *prev;
  struct _scandev_device *next;
} SCANDEV_T;

/*
   add a scanned device to the list
*/
bool ScanDevAdd(const BLEAddress addr, const char *name, const uint16_t manufacturer_id, const int rssi, const bool has_battery);

/*
   return the BLE scan list as HTML

   the callback expects each device as an HTML chunk
*/
void ScanDevListHTML(void (*callback)(const String& content));

/*
   setup the bluetooth stuff
*/
void ScanDevSetup(void);

/*
   do the cyclic update
*/
void ScanDevUpdate(void);

#endif

/**/
