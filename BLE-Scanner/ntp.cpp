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

#include "config.h"
#include "ntp.h"
#include "util.h"

/*
   NTP configuration
*/
static CONFIG_NTP_T _config_ntp;

/*
**  local port to listen for UDP packets
*/
#define NTP_UDP_LOCAL_PORT 8888

/*
**  global port to receive the UDP packets from
*/
#define NTP_UDP_PORT 123

/*
**  NTP time sync interval
*/
#define NTP_SYNC_INTERVAL  (60 * 5)

/*
**  NTP time stamp is in the first 48 bytes of the message
*/
#define NTP_PACKET_SIZE 48

/*
**  NTP packet buffer
*/
static byte _packetBuffer[NTP_PACKET_SIZE];

/*
   some NTP configuration
*/
static String _ntp_server = "";
static IPAddress _ntp_ip(0, 0, 0, 0);
static int _ntp_sync_cycle = 0;
static time_t _up_since = 0;

/*
**  UDP instance to let us send and receive packets
*/
static WiFiUDP _Udp;

/*
**  send an NTP request to the time server at the given address
**
**  NOTE: function is called asynchronous, so don't use LogMsg or Serial!
*/
static void NtpSendRequest(void)
{
  memset(_packetBuffer, 0, NTP_PACKET_SIZE);

  _packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  _packetBuffer[1] = 0;     // Stratum, or type of clock
  _packetBuffer[2] = 6;     // Polling Interval
  _packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  _packetBuffer[12]  = 49;
  _packetBuffer[13]  = 0x4E;
  _packetBuffer[14]  = 49;
  _packetBuffer[15]  = 52;

  _Udp.beginPacket(_ntp_ip, NTP_UDP_PORT);
  _Udp.write(_packetBuffer, NTP_PACKET_SIZE);
  _Udp.endPacket();
}

/*
**  handle a received NTP reply
**
**  NOTE: function is called asynchronous, so don't use LogMsg or Serial!
*/
static time_t NtpReceiveReply(void)
{
  /*
  **    read the packet
  */
  _Udp.read(_packetBuffer, NTP_PACKET_SIZE);

  /*
     NTP time is in seconds since Jan 1 1900
  */
  unsigned long highWord = word(_packetBuffer[40], _packetBuffer[41]);
  unsigned long lowWord = word(_packetBuffer[42], _packetBuffer[43]);
  // combine the four bytes (two words) into a long integer
  // this is NTP time (seconds since Jan 1 1900):
  unsigned long ntpTime = highWord << 16 | lowWord;
  // now convert NTP time into UNIX time (seconds since Jan 1 1970)
  ntpTime -= 2208988800UL;
  //DbgMsg("NTP: time received: %lu", ntpTime);
  if (!_up_since)
    _up_since = ntpTime;
  return ntpTime;
}

/*
**  sent an NTP request and wait for the answer
**
**  NOTE: function is called asynchronous, so don't use LogMsg or Serial!
**
**  NOTE: we will wait for a maximum of 200 * 10ms
*/
static time_t NtpSync(void)
{
  NtpSendRequest();

  for (int retry = 0; retry < 200; retry++) {
    if (_Udp.parsePacket()) {
      return NtpReceiveReply();
    }
    delay(10);
  }
  return 0;
}

/*
   initialize ntp
*/
static void NtpInit()
{
  if (_ntp_ip[0]) {
    _Udp.begin(NTP_UDP_LOCAL_PORT);
    setSyncInterval((unsigned int) NTP_SYNC_INTERVAL);
    setSyncProvider(NtpSync);
  }
}

/*
**	init the ntp functionality
*/
void NtpSetup(void)
{
  if (StateCheck(STATE_CONFIGURING))
    return;

  /*
     get the NTP from the configuration
  */
  CONFIG_GET(NTP, ntp, &_config_ntp);
  if (!_config_ntp.server[0]) {
    LogMsg("NTP: no server configured");
    return;
  }
  LogMsg("NTP: setting up NTP, server=%s", _config_ntp.server);

  /*
     look up the ntpservers ip
  */
  if (WiFi.hostByName(_config_ntp.server, _ntp_ip)) {
    /*
       we could resolve the ntp name, so store it in the flash
    */
#if DBG_NTP
    DbgMsg("NTP: lookup of %s successful: %s", _config_ntp.server, IPAddressToString(_ntp_ip).c_str());
#endif
  }
  else {
#if DBG_NTP
    DbgMsg("NTP: lookup of %s failed", _config_ntp.server);
#endif
    return;
  }

#if DBG_NTP
  DbgMsg("NTP: ip=%s", IPAddressToString(_ntp_ip).c_str());
#endif

  NtpInit();
}

/*
**  update the NTP time
*/
void NtpUpdate(void)
{
  if (StateCheck(STATE_CONFIGURING))
    return;

  if (++_ntp_sync_cycle >= NTPSYNC_CYCLES && timeStatus() != timeSet) {
    _ntp_sync_cycle = 0;
    NtpInit();
  }
}

/*
   get the uptime
*/
time_t NtpUptime(void)
{
  return now() - _up_since;
}

/*
   get the frist received timestamp
*/
time_t NtpUpSince(void)
{
  return _up_since;
}/**/
