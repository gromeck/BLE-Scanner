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
#include "macaddr.h"

/*
   how mandy device should be keep in our list
*/
#define SCANDEV_LIST_MAX_LENGTH    1000

/*
   how many chars should be store from the name of a device

*/
#define SCANDEV_NAME_LENGTH        20


typedef enum {
  SCANDEV_TYPE_BTC = 0,
  SCANDEV_TYPE_BLE,
} SCANDEV_TYPE_T;


/*
   struct to hold a found BLE device
*/
typedef struct _scandev_device {
  SCANDEV_TYPE_T devtype;
  byte mac[MAC_ADDR_LEN];
  int rssi;
  int cod;
  char name[SCANDEV_NAME_LENGTH + 1];
  const char *vendor;
  time_t last_seen;
  bool present;
  bool publish;
  time_t last_published;
  struct _scandev_device *prev;
  struct _scandev_device *next;
} SCANDEV_T;

/*
   add a scanned device to the list
*/
bool ScanDevAdd(SCANDEV_TYPE_T devtype, const byte * mac, const char *name, const int rssi, const int cod);

/*
   return the last BLE scan list as HTML
*/
String ScanDevListHTML(void);

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
