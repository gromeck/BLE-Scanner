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

#ifndef __BLE_MANUFACTURER_H__
#define __BLE_MANUFACTURER_H__ 1

#include "util.h"

#define BLE_MANUFACTURER_ID_UNKNOWN   0xffff

/*
   setup
*/
void BLEManufacturerSetup(void);

/*
   lookup the vendor by the mac address
*/
const char *BLEManufacturerLookup(const uint16_t id, const char *none);

/*
   return a static string with the manufacturer ID
*/
const char *BLEManufacturerIdHex(const uint16_t id);

#endif
