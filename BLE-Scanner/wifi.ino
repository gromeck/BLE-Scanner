/*
  BLE-Scanner

  (c) 2020 Christian.Lorenz@gromeck.de

  module to handle the WiFi stack


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
#include "wifi.h"

WiFiClient _wifiClient;
static DNSServer *_dns_server = NULL;
static char _config_mode_SSID[64] = "";

/*
   setup wifi
*/
bool WifiSetup(void)
{
  LogMsg("WIFI: my MAC address is %s", WifiGetMacAddr().c_str());

  if (_config_mode) {
    /*
       open an AccessPoint
    */
    uint8_t mac[6];
    strcpy(_config_mode_SSID,(String(__TITLE__ "-AP-") + String(AddressToString((byte *) WiFi.macAddress(mac) + sizeof(mac) - WIFI_AP_USE_LAST_MAC_DIGITS, WIFI_AP_USE_LAST_MAC_DIGITS, false))).c_str());

    LogMsg("WIFI: opening access point with SSID %s ...", _config_mode_SSID);
    WiFi.softAP(_config_mode_SSID);
    delay(1000);

    LogMsg("WIFI: local IP address %s", IPAddressToString(WiFi.softAPIP()).c_str());

    /*
     * configure the DNS so that every requests will go to our device
     */
    _dns_server = new DNSServer();
    _dns_server->setErrorReplyCode(DNSReplyCode::NoError);
    _dns_server->start(DNS_PORT, "*", WiFi.softAPIP());
  }
  else {
    /*
      connect to the configured Wifi network
    */
    DbgMsg("WIFI: SSID=%s  PSK=%s", _config.wifi.ssid, _config.wifi.psk);

    WiFi.mode(WIFI_STA);
    WiFi.begin(_config.wifi.ssid, _config.wifi.psk);

    LogMsg("WIFI: waiting to connect to %s ...", WiFi.SSID().c_str());
  }

  return WifiUpdate();
}

/*
   do Wifi updates
*/
bool WifiUpdate(void)
{
  if (_config_mode) {
    /*
       we are in access point mode

       as we are the DNS, we have to answer the requests
    */
    if (_dns_server)
      _dns_server->processNextRequest();
  }
  else {
    /*
       normal operation mode
    */
    if (WiFi.status() != WL_CONNECTED) {
      /*
         wait to connect
      */
      int retries = 20;

      DbgMsg("WIFI: status is not connected ... waiting for connection");
      while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        if (--retries <= 0) {
          LogMsg("WIFI: giving up");
          return false;
        }
      }

      /*
         up an running
      */
      IPAddress ip = WiFi.localIP();
      LogMsg("WIFI: connected to %s with local IP address %s", _config.wifi.ssid, IPAddressToString(ip).c_str());
    }
  }
  return true;
}

/*
   return the SSID
*/
String WifiGetSSID(void)
{
  return _config_mode ? _config_mode_SSID : WiFi.SSID();
}

/*
   return the SSID
*/
int WifiGetRSSI(void)
{
  return WiFi.RSSI();
}


/*
   return the SSID
*/
String WifiGetIpAddr(void)
{
  return IPAddressToString(_config_mode ? WiFi.softAPIP() : WiFi.localIP());
}

/*
   return the Wifi MAC address
*/
String WifiGetMacAddr(void)
{
  uint8_t mac[6];

  return String(AddressToString((byte *) WiFi.macAddress(mac), sizeof(mac), false));
}


WiFiClient *WifiGetClient(void)
{
  return &_wifiClient;
}/**/
