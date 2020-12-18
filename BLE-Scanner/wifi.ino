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

static CONFIG_WIFI _config_wifi;
WiFiClient _wifiClient;
static DNSServer *_dns_server = NULL;

/*
   setup wifi
*/
void WifiSetup(void)
{
  CONFIG_GET(WIFI, wifi, &_config_wifi);

  LogMsg("WIFI: my MAC address is %s", WifiGetMacAddr().c_str());

  if (_config_mode) {
    /*
       open an AccessPoint
    */
    uint8_t mac[6];
    String SSID = String(__TITLE__ "-AP-") + String(AddressToString((byte *) WiFi.macAddress(mac) + sizeof(mac) - WIFI_AP_USE_LAST_MAC_DIGITS, WIFI_AP_USE_LAST_MAC_DIGITS, false));

    LogMsg("WIFI: opening access point with SSID %s ...", SSID.c_str());
    WiFi.softAP(SSID.c_str());
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
    DbgMsg("WIFI: SSID=%s  PSK=%s", _config_wifi.ssid, _config_wifi.psk);

    WiFi.mode(WIFI_STA);
    WiFi.begin(_config_wifi.ssid, _config_wifi.psk);

    LogMsg("WIFI: waiting to connect to %s ...", WiFi.SSID().c_str());
  }

  WifiUpdate();
}

/*
   do Wifi updates
*/
void WifiUpdate(void)
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
      DbgMsg("WIFI: status is not connected ... waiting for connection");
      while (WiFi.status() != WL_CONNECTED) {
        delay(500);
      }

      /*
         up an running
      */
      IPAddress ip = WiFi.localIP();
      LogMsg("WIFI: connected to %s with local IP address %s", _config_wifi.ssid, IPAddressToString(ip).c_str());
    }
  }
}

/*
   return the SSID
*/
String WifiGetSSID(void)
{
  return WiFi.SSID();
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
  return IPAddressToString(_config_mode ? WiFi.localIP() : WiFi.softAPIP());
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
