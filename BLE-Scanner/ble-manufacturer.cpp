/*
  BLE-Scanner

  (c) 2020 Christian.Lorenz@gromeck.de

  module to handle the MQTT stuff


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

#include "config.h"
#include "ble-manufacturer.h"

/*
   struct to keep one mac address and its vendor
*/
typedef struct _ble_manufacturer {
  uint16_t id;
  const char *name;
} BLE_MANUFACTURER;

/*
   array of known mac address with their vendors
*/
static const BLE_MANUFACTURER _ble_manufacturers[] PROGMEM = {
#if __has_include("ble-manufacturer-list.h")
#include "ble-manufacturer-list.h"
#endif
  { 0, 0 }
};

/*
   define the list lenght
*/
#define BLE_MANUFACTURER_LIST_LENGTH   (sizeof(_ble_manufacturers) / sizeof(_ble_manufacturers[0]) - 1)

/*
   setup
*/
void BLEManufacturerSetup(void)
{
#if DBG_MAC
  DbgMsg("BLEManufacturerSetup: sizeof lookup table entry: %d", sizeof(BLE_MANUFACTURER));
  DbgMsg("BLEManufacturerSetup: sizeof lookup table: %d", sizeof(_ble_manufacturers));
  DbgMsg("BLEManufacturerSetup: length of lookup table: %d", BLE_MANUFACTURER_LIST_LENGTH);
#endif
}

/*
   lookup the vendor by the mac address
*/
const char *BLEManufacturerLookup(const uint16_t id, const char *none)
{
#if DBG_MANUFACTURER
  DbgMsg("BLEManufacturerLookup: looking up %04x", id);
#endif

  int low = 0;
  int high = BLE_MANUFACTURER_LIST_LENGTH - 1;
  int mid, cmp;

  while (low <= high) {
    mid = low + (high - low) / 2;
    if ((cmp = (long) id - (long) _ble_manufacturers[mid].id))
      if (cmp > 0)
        low = mid + 1;
      else
        high = mid - 1;
    else
      return _ble_manufacturers[mid].name;
  }
#if DBG_BLE_MANUFACTURER
  DbgMsg("BLEManufacturerLookup: nothing found");
#endif
  return none;
}

/*
   return a static string with the manufacturer ID
*/
const char *BLEManufacturerIdHex(const uint16_t id)
{
  static char _hexid[8];

  sprintf(_hexid, "0x%04X", id);
  return _hexid;
}/**/
