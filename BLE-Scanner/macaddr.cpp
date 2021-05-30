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
   struct to keep one mac address and its vendor
*/
typedef struct _macaddr {
  const byte mac[3];
  const char *vendor;
} MACADDR;

/*
   array of known mac address with their vendors
*/
static const MACADDR _macaddrs[] PROGMEM = {
//#include "macaddr-list.h"
  { 0, 0 }
};

/*
   define the list lenght
*/
#define MACADDR_LIST_LENGTH   (sizeof(_macaddrs) / sizeof(_macaddrs[0]) - 1)

/*
   compare two mac addresses (first 3 digits
*/
static int maccmp(const byte *a, const byte *b)
{
  int cmp;

  if ((cmp = (int) * a++ - (int) * b++))
    return cmp;
  if ((cmp = (int) * a++ - (int) * b++))
    return cmp;
  return (int) * a - (int) * b;
}

/*
   setup
*/
void MacAddrSetup(void)
{
#if DBG_MAC
  DbgMsg("MAC: sizeof lookup table entry: %d", sizeof(MACADDR));
  DbgMsg("MAC: sizeof lookup table: %d", sizeof(_macaddrs));
  DbgMsg("MAC: length of lookup table: %d", MACADDR_LIST_LENGTH);
#endif
}

/*
   lookup the vendor by the mac address
*/
const char *MacAddrLookup(const byte *mac,const char *none)
{
#if DBG_MAC
  DbgMsg("MAC: looking up %02x:%02x:%02x", mac[0], mac[1], mac[2]);
#endif

  int low = 0;
  int high = MACADDR_LIST_LENGTH - 1;
  int mid, cmp;

  while (low <= high) {
    mid = low + (high - low) / 2;
    if ((cmp = maccmp(mac, _macaddrs[mid].mac)))
      if (cmp > 0)
        low = mid + 1;
      else
        high = mid - 1;
    else
      return _macaddrs[mid].vendor;
  }
#if DBG_MAC
  DbgMsg("MAC: nothing found");
#endif
  return none;
}/**/
