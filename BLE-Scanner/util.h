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

#include <TimeLib.h>
#include "wifi.h"

#if DBG
/*
**  debug message
*/
#define DbgMsg  LogMsg
#else
#define DbgMsg(...)
#endif

/*
   compute the distance out of the RSSI value
*/
#define RSSI2METER(rssi)  (round(pow(10.0,((-69.0 - (rssi)) / (10.0 * 2.0))) * 100.0) / 100.0)

/*
   check/fix the range of a value
*/
#define MIN(a,b)    (((a) < (b)) ? (a) : (b))
#define MAX(a,b)    (((a) > (b)) ? (a) : (b))
#define CHECK_RANGE(value,lower,upper)  MAX(MIN((value),(upper)),(lower))
#define FIX_RANGE(value,lower,upper)  { value = CHECK_RANGE((value),(lower),(upper)); }

#ifndef SECS_PER_HOUR
#define SECS_PER_HOUR   (60 * 60)
#endif

#define MAC_ADDR_LEN  6

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
   get a time in ascii
*/
const char *TimeToString(time_t t);

/*
**  log a smessage to serial
*/
void LogMsg(const char *fmt, ...);

#endif

/**/
