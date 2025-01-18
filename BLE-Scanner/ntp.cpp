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


  see https://tools.ietf.org/rfc/rfc5905 for the NTP protocol reference.
*/

#include "config.h"
#include "wifi.h"
#include "ntp.h"
#include "util.h"

/*
   NTP configuration
*/
static CONFIG_NTP_T _config_ntp;


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
static unsigned long _first_sync = 0;
static unsigned long _last_sync = 0;
static unsigned long _last_request = 0;
static long _last_correction = 0;

/*
    some NTP statistics
*/
static int _ntp_requests = 0;
static int _ntp_replies_total = 0;
static int _ntp_replies_good = 0;

#if DBG_NTP
static bool _ntp_received = false;
#endif

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
  if (!_last_request || (millis() - _last_request > NTP_SYNC_INTERVAL * 1000) || millis() < _last_request) {
    _last_request = millis();
#if DBG_NTP
    _ntp_received = false;
#endif
    memset(_packetBuffer, 0, NTP_PACKET_SIZE);

    _packetBuffer[0] = 0b11100011;// LI=3 (clock unsynchronized, 2 bit), Version=4 (3 bit), Mode=3 (=client, 3 bit)
    _packetBuffer[1] = 0;         // Stratum, or type of clock
    _packetBuffer[2] = NTP_POLL;  // Polling Interval
    _packetBuffer[3] = 0xec;      // Peer Clock Precision=-20 (2 ^ -20 = ~ 1 microsecond)
    // 8 bytes of zero for Root Delay & Root Dispersion
    _packetBuffer[12]  = '1';
    _packetBuffer[13]  = 'N';
    _packetBuffer[14]  = '1';
    _packetBuffer[15]  = '4';

    _Udp.begin(NTP_UDP_LOCAL_PORT);

    _Udp.beginPacket(_ntp_ip, NTP_UDP_PORT);
    _Udp.write(_packetBuffer, NTP_PACKET_SIZE);
    _Udp.endPacket();

    _ntp_requests++;
  }
}

/*
**  handle a received NTP reply
**
**  NOTE: function is called asynchronous, so don't use LogMsg or Serial!
*/
static time_t NtpReceiveReply(void)
{
  /*
  **  read the packet
  */
  _Udp.read(_packetBuffer, NTP_PACKET_SIZE);
  _ntp_replies_total++;

  /*
  **  if the stratum field is 0, the answer is not what we expect
  */
  if (_packetBuffer[1] == 0)     // Stratum, or type of clock
    return 0;

  /*
  **  NTP time is in seconds since Jan 1 1900
  */
  unsigned long highWord = word(_packetBuffer[40], _packetBuffer[41]);
  unsigned long lowWord = word(_packetBuffer[42], _packetBuffer[43]);
  unsigned long ntpTime = highWord << 16 | lowWord;

  if (!ntpTime)
    return 0;

  /*
  **  now convert NTP time into UNIX time (seconds since Jan 1 1970)
  **
  **  the offset is 70 y * ~365.242857142 d * 24 h * 60 m * 60 s
  */
  ntpTime -= 2208988800UL;

  /*
  **  we only accecpt the time, when the delta is below our limit
  */
  long delta = ntpTime - now();
  if (_first_sync) {
    /*
    **  this is an update, so drift the time
    */
    ntpTime = now() + ((delta < 0) ? -1 : 1) * min((long) NTP_MAX_DRIFT,labs(delta));
  }
  else
    _first_sync = ntpTime;
  _last_sync = ntpTime;
  _last_correction = ntpTime - now();
  _ntp_replies_good++;

#if DBG_NTP
  _ntp_received = true;
#endif

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
       we could resolve the ntp name
    */
#if DBG_NTP
    DbgMsg("NTP: lookup of %s successful: %s", _config_ntp.server, IPAddressToString(_ntp_ip).c_str());
#endif

    /*
        configure the cyclic update via the Time library
    */
    setSyncInterval((unsigned int) NTP_SYNC_INTERVAL);
    setSyncProvider(NtpSync);
  }
#if DBG_NTP
  else {
    DbgMsg("NTP: lookup of %s failed", _config_ntp.server);
  }
#endif
}

/*
**  update the NTP time
*/
void NtpUpdate(void)
{
  if (StateCheck(STATE_CONFIGURING))
    return;

#if DBG_NTP
  static unsigned long _last_stats = 0;
  
  if (millis() - _last_stats > NTP_SYNC_INTERVAL / 2 * 1000 || millis() < _last_stats) {
    /*
        print some stats
    */
    _last_stats = millis();
    DbgMsg("NTP: stats: requests=%d  replies=%d  good replies=%d",_ntp_requests,_ntp_replies_total,_ntp_replies_good);
  }
  
  if (_ntp_received) {
    /*
    **  this section help to debug the incoming NTP packets
    **  as inside the receiver function the DbgMsg or LogMsg calls
    **  are not possible. Instead we use the flag _ntp_receive
    **  whenever the received packet is valid
    */
    dump("NTP: packet received",(const void *) &_packetBuffer[0],NTP_PACKET_SIZE);

    /*
    **  NTP time is in seconds since Jan 1 1900
    */
    unsigned long highWord = word(_packetBuffer[40], _packetBuffer[41]);
    unsigned long lowWord = word(_packetBuffer[42], _packetBuffer[43]);
    unsigned long ntpTime = highWord << 16 | lowWord;
    DbgMsg("NTP: packet received: highword=%lu  lowWord=%lu  ntpTime=%lu",highWord,lowWord,ntpTime);

    if (ntpTime) {
      /*
      **  now convert NTP time into UNIX time (seconds since Jan 1 1970)
      **
      **  the offset is 70 y * ~365.242857142 d * 24 h * 60 m * 60 s
      */
      ntpTime -= 2208988800UL;
      DbgMsg("NTP: packet received: ntpTime=%lu  now=%lu",ntpTime,now());

      /*
      **  we only accecpt the time, when the delta is below our limit
      */
      long delta = ntpTime - now();
      if (_first_sync) {
        /*
        **  this is an update, so drift the time
        */
        ntpTime = now() + ((delta < 0) ? -1 : 1) * min((long) NTP_MAX_DRIFT,labs(delta));
        DbgMsg("NTP: packet received: delta=%ld  _first_sync=%lu => ntpTime=%lu", delta,_first_sync,ntpTime);
      }
      DbgMsg("NTP: packet received: last_correction=%ld",_last_correction);
    }
    _ntp_received = false;
  }
#endif
}

/*
   get the uptime
*/
unsigned long NtpUptime(void)
{
  return now() - _first_sync;
}

/*
   get the frist received timestamp
*/
time_t NtpFirstSync(void)
{
  return _first_sync;
}

/*
   get the last received timestamp
*/
time_t NtpLastSync(void)
{
  return _last_sync;
}

/*
   get the last delta between the ntp and our localtime
*/
long NtpLastCorrection(void)
{
  return _last_correction;
}

/*
   get some stats
*/
void NtpStats(int *requests,int *replies_total,int *replies_good)
{
  if (requests)
    *requests = _ntp_requests;
  if (replies_total)
    *replies_total = _ntp_replies_total;
  if (replies_good)
    *replies_good = _ntp_replies_good;
}/**/
