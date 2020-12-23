/*
  BLE-Scanner

  (c) 2020 Christian.Lorenz@gromeck.de

  module to provide utility functions


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

#ifndef __UTIL_H__
#define __UTIL_H__ 1

#include "wifi.h"
#include "time.h"

#if DBG
/*
**  debug message
*/
#define DbgMsg  LogMsg
#else
#define DbgMsg(...)
#endif

/*
   convert an IP address to a C string
*/
#define IPAddressToString(addr)   ((addr).toString())

/*
   convert an IP address to a field of bytes
*/
const byte *IPAddressToBytes(IPAddress &addr);

/*
   convert a field of bytes to an IP address
*/
IPAddress BytesToIPAddress(byte *bytes);

/*
**  return a given IP or HW address as a string
*/
const char *AddressToString(byte *addr, int addrlen, bool dec, char sep);


/*
**  convert the string into an IP or HW address
*/
const byte *StringToAddress(const char *str, int addrlen, bool dec);

/*
   dump data from an address
*/
void dump(String title, const void *addr, const int len);

/*
 * get a time in ascii
 */
const char *TimeToString(time_t t);

/*
**  log a smessage to serial
*/
void LogMsg(const char *fmt, ...);

#endif

/**/
