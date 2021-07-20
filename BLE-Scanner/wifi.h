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

#ifndef __WIFI_H__
#define __WIFI_H__  1

#include <WiFi.h>
#include <WiFiUdp.h>
#include <WiFiClient.h>
#include <DNSServer.h>
#include "config.h"
#include "state.h"

extern WiFiClient _wifiClient;

/*
   the WiFi SSID for configuration mode
*/
#define WIFI_AP_SSID_PREFIX                __TITLE__ "-AP-"

/*
   the WiFi SSID for configuration mode uses the last digits of the own MAC
*/
#define WIFI_AP_SSID_USE_LAST_MAC_DIGITS   3

/*
   number of retries to connect to the configured Wifi

   when retrie count is reached, we will give up -- the device will enter the configuration mode
*/
#define WIFI_CONNECT_RETRIES          20

/*
   port for the DNS
*/
#define DNS_PORT  53

/*
   compute the WiFi signal strength in percent out of the RSSI
*/
#define WIFI_RSSI_TO_QUALITY(rssi)  max(0,min(100,2 * (100 + rssi)))

/*
   setup wifi
*/
bool WifiSetup(void);

/*
   do Wifi updates
*/
bool WifiUpdate(void);

/*
   return the SSID
*/
String WifiGetSSID(void);

/*
   return the SSID
*/
int WifiGetChannel(void);

/*
   return the SSID
*/
int WifiGetRSSI(void);

/*
   return the own IP address
*/
String WifiGetIpAddr(void);

/*
   return the Wifi MAC address
*/
String WifiGetMacAddr(void);

/*
   get the WiFi client object for Wifi users
*/
WiFiClient *WifiGetClient(void);

#endif

/**/
