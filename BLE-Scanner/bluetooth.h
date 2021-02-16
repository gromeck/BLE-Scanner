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

#include "config.h"
#include "macaddr.h"

/*
   how mandy device should be keep in our list
*/
#define BLUETOOTH_DEVICE_LIST_MAX_LENGTH    1000

/*
   how many chars should be store from the name of a device

*/
#define BLUETOOTH_DEVICE_NAME_LENGTH        20


typedef enum {
  BLUETOOTH_MODE_IDLE = 0,
  BLUETOOTH_MODE_BTC_STARTED,
  BLUETOOTH_MODE_BTC_STOPPED,
  BLUETOOTH_MODE_BLE_STARTED,
  BLUETOOTH_MODE_BLE_STOPPED,
} bluetooth_mode;

typedef enum {
  BLUETOOTH_DEVTYPE_BTC = 0,
  BLUETOOTH_DEVTYPE_BLE,
} bluetooth_devtype;

/*
    Bluetooth LE settings (in seconds)
*/
#define BLUETOOTH_BTC_SCAN_TIME_MIN     0
#define BLUETOOTH_BTC_SCAN_TIME_MAX     (3 * 60)
#define BLUETOOTH_BLE_SCAN_TIME_MIN     0
#define BLUETOOTH_BLE_SCAN_TIME_MAX     (3 * 60)
#define BLUETOOTH_PAUSE_TIME_MIN        0
#define BLUETOOTH_PAUSE_TIME_MAX        (24 * 60 * 60)
#define BLUETOOTH_ABSENCE_CYCLES_MIN    1
#define BLUETOOTH_ABSENCE_CYCLES_MAX    10

/*
   struct to hold a found BLE device
*/
typedef struct _bluetooth_device {
  bluetooth_devtype devtype;
  byte mac[MAC_ADDR_LEN];
  int rssi;
  int cod;
  char name[BLUETOOTH_DEVICE_NAME_LENGTH + 1];
  const char *vendor;
  time_t last_seen;
  bool present;
  bool publish;
  struct _bluetooth_device *prev;
  struct _bluetooth_device *next;
} BLUETOOTH_DEVICE;


/*
   return the last BLE scan list as HTML
*/
String BluetoothScanListHTML(void);

/*
 * setup the bluetooth stuff
 */
void BluetoothSetup(void);

/*
 * do the cyclic update
 */
void BluetoothUpdate(void);

/*
   init the scanning
*/
bool BluetoothStartScan(void);

#endif

/**/
