/*
  BLE-Scanner

  (c) 2020 Christian.Lorenz@gromeck.de

  module to handle the NTP protocol


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

#ifndef __NTP_H__
#define __NTP_H__ 1

#include <TimeLib.h>
#include "config.h"
#include "util.h"

/*
**  local port to listen for UDP packets
*/
#define NTP_UDP_LOCAL_PORT 8888

/*
**  global port to receive the UDP packets from
*/
#define NTP_UDP_PORT 123

/*
**  NTP retry interval if failed
*/
#define NTP_RETRY_INTERVAL  5

/*
**  NTP drift maximum
*/
#define NTP_MAX_DRIFT       10

/*
**  NTP time sync interval
**
**  the poll intervall is typically 2^6 (= 64 s = ~1 min) to 2^10 (= 1014 s = ~17 min) senconds
*/
#define NTP_POLL            (DBG_NTP ? 6 : 10)
#define NTP_SYNC_INTERVAL   (1 << (NTP_POLL))


/*
**  init the NTP functions
*/
void NtpSetup(void);

/*
**  update the NTP time
*/
void NtpUpdate(void);

/*
**  get the NTTP time
*/
time_t NtpGetTime(void);

/*
   get the uptime in seconds since first ntp received
*/
unsigned long NtpUptime(void);

/*
   get the frist received timestamp
*/
time_t NtpFirstSync(void);

/*
   get the last received timestamp
*/
time_t NtpLastSync(void);

/*
   get the last correction in seconds
*/
long NtpLastCorrection(void);

/*
   get some stats
*/
void NtpStats(int *requests,int *replies_total,int *replies_good);

#endif

/**/