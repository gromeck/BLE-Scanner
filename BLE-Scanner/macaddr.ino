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
#include "macaddr.h"

/*
 * struct to keep one mac address and its vendor
 */
typedef struct _macaddr {
  unsigned char mac[4];
  const char *vendor;
} MACADDR;

/*
 * array of known mac address with their vendors
 */
static MACADDR _macaddrs[] PROGMEM = {
#include "macaddr-list.h"
  { 0, 0 }
};

/*
 * lookup the vendor by the mac address
 */
const char *MacAddrLookup(const byte *mac)
{
  LogMsg("MAC: looking up %02x:%02x:%02x",mac[0],mac[1],mac[2]);
  LogMsg("MAC: scanning %d vendors",sizeof(_macaddrs) / sizeof(_macaddrs[0]));
  for (int n = 0;n < sizeof(_macaddrs) / sizeof(_macaddrs[0]);n++) {
    if (mac[0] == _macaddrs[n].mac[0] && mac[1] == _macaddrs[n].mac[1] && mac[2] == _macaddrs[n].mac[2])
      return _macaddrs[n].vendor;
  }
  LogMsg("MAC: nothing found");
  return NULL;
}/**/
