/*
  BLE-Scanner

  (c) 2020 Christian.Lorenz@gromeck.de

  module to handle the HTTP stuff


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
#include "http.h"
#include "wifi.h"
#include "ntp.h"
#include <WebServer.h>

/*
   the web server object
*/
static WebServer _WebServer(80);

/*
  time of the last HTTP request
*/
static unsigned long _last_request = 0;

/*
   setup the webserver
*/
void HttpSetup(void)
{
  static String _html_header PROGMEM =
    "<!DOCTYPE html>"
    "<html>"
    "<head>"
    "<meta charset='utf-8'>"
    "<meta name='viewport' content='width=device-width,initial-scale=1,user-scalable=no'>"
    "<title>" __TITLE__ "</title>"
    "<link href='/styles.css' rel='stylesheet' type='text/css'>"
    "</head>"
    "<body>"
    "<div class=content>"
    "<div class=header>"
    "<h3>" __TITLE__ "</h3>"
    "<h2>" + String(_config.device.name) + "</h2>"
    "</div>"
    ;
  static String _html_footer PROGMEM =
    "<div class=footer>"
    "<hr>"
    "<a href='https://github.com/gromeck/BLE-Scanner' target='_blank' style='color:#aaa;'>" __TITLE__ " Version: " GIT_VERSION "</a>"
    "</div>"
    "</div>"
    "</body>"
    "</html>"
    ;

  LogMsg("HTTP: setting up HTTP server");

  /*
     get the current config as a duplicate
  */
  ConfigGet(0, sizeof(CONFIG), &_config);

  _WebServer.onNotFound( []() {
    _last_request = millis();
    if (!StateCheck(STATE_CONFIGURING) && _config.device.password[0] && !_WebServer.authenticate(HTTP_WEB_USER, _config.device.password))
      return _WebServer.requestAuthentication();

    _WebServer.send(200, "text/html",
                    _html_header +
                    "<form action='/config' method='get'><button>Configuration</button></form><p>"
                    "<form action='/info' method='get'><button>Information</button></form><p>"
                    "<form action='/restart' method='get' onsubmit=\"return confirm('Are you sure to restart the device?');\"><button class='button redbg'>Restart</button></form><p>"
                    "<p>"
                    "<form action='/ble' method='get'><button>BLE Scan List</button></form><p>"
                    + _html_footer);
  });

  _WebServer.on("/styles.css", []() {
    _last_request = millis();
    _WebServer.send(200, "text/css",
                    "html, body { background:#ffffff; }"
                    "body { margin:1rem; padding:0; font-familiy:'sans-serif'; color:#202020; text-align:center; font-size:1rem; }"
                    "input { width:100%; font-size:1rem; }"
                    "button { border: 0; border-radius: 0.3rem; background: #1881ba; color: #ffffff; line-height: 2.4rem; font-size: 1.2rem; width: 100%; -webkit-transition-duration: 0.5s; transition-duration: 0.5s; cursor: pointer; opacity:0.8; }"
                    "button:hover { opacity: 1.0; }"
                    ".header { text-align:center; }"
                    ".content { text-align:left; display:inline-block; color:#000000; min-width:340px; }"
                    ".msg { text-align:center; color:#be3731; font-weight:bold; padding:5rem 0; }"
                    ".blescanlist { padding:0; margin:0; width: 100%; }"
                    ".blescanlist tr td { font-familiy:'monospace'; }"
                    ".blescanlist tr td { padding:4px; }"
                    ".blescanlist tr:nth-child(even) { background: #c0c0c0; }"
                    ".blescanlist tr:nth-child(odd) {background: #ffffff; }"
                    ".footer { text-align:right; }"
                    ".greenbg { background: #348f4b; }"
                    ".redbg { background: #a12828; }"
                   );
  });

  _WebServer.on("/config", []() {
    _last_request = millis();
    if (!StateCheck(STATE_CONFIGURING) && _config.device.password[0] && !_WebServer.authenticate(HTTP_WEB_USER, _config.device.password))
      return _WebServer.requestAuthentication();

    /*
       handle configuration changes
    */
#if DBG
    for (int n = 0; n < _WebServer.args(); n++ )
      LogMsg("HTTP: args: %s=%s", _WebServer.argName(n).c_str(), _WebServer.arg(n).c_str());
#endif

    if (_WebServer.hasArg("save")) {
      /*
         take over the configuration parameters
      */
#define CHECK_AND_SET_STRING(type,name) { if (_WebServer.hasArg(#type "_" #name)) strncpy(_config.type.name,_WebServer.arg(#type "_" #name).c_str(),sizeof(_config.type.name) - 1); }
#define CHECK_AND_SET_NUMBER(type,name,minimum,maximum) { if (_WebServer.hasArg(#type "_" #name)) _config.type.name = min(max(atoi(_WebServer.arg(#type "_" #name).c_str()),(minimum)),(maximum)); }
      CHECK_AND_SET_STRING(device, name);
      CHECK_AND_SET_STRING(device, password);
      CHECK_AND_SET_STRING(wifi, ssid);
      CHECK_AND_SET_STRING(wifi, psk);
      CHECK_AND_SET_STRING(ntp, server);
      CHECK_AND_SET_STRING(mqtt, server);
      CHECK_AND_SET_NUMBER(mqtt, port, 1024, 65535);
      CHECK_AND_SET_STRING(mqtt, user);
      CHECK_AND_SET_STRING(mqtt, password);
      CHECK_AND_SET_STRING(mqtt, clientID);
      CHECK_AND_SET_NUMBER(ble, scan_time, BLE_SCAN_TIME_MIN, BLE_SCAN_TIME_MAX);
      CHECK_AND_SET_NUMBER(ble, pause_time, BLE_PAUSE_TIME_MIN, BLE_PAUSE_TIME_MAX);

      /*
         write the config back
      */
      ConfigSet(0, sizeof(CONFIG), &_config);
    }

    _WebServer.send(200, "text/html",
                    _html_header +
                    "<form action='/config/device' method='get'><button>Configure Device</button></form><p>"
                    "<form action='/config/wifi' method='get'><button>Configure WiFi</button></form><p>"
                    "<form action='/config/ntp' method='get'><button>Configure NTP</button></form><p>"
                    "<form action='/config/mqtt' method='get'><button>Configure MQTT</button></form><p>"
                    "<form action='/config/ble' method='get'><button>Configure BLE Scan</button></form><p>"
                    "<form action='/config/reset' method='get' onsubmit=\"return confirm('Are you sure to reset the configuration?');\"><button class='button redbg'>Reset configuration</button></form><p>"
                    "<p><form action='/' method='get'><button>Main Menu</button></form><p>"
                    + _html_footer);
  });

  _WebServer.on("/config/device", []() {
    _last_request = millis();
    _WebServer.send(200, "text/html",
                    _html_header +
                    "<form method='get' action='/config'>"
                    "<fieldset>"
                    "<legend>"
                    "<b>&nbsp;Device&nbsp;</b>"
                    "</legend>"
                    "<b>Name</b>"
                    "<br>"
                    "<input name='device_name' type='text' placeholder='Device name' value='" + String(_config.device.name) + "'>"
                    "<p>"
                    "<b>Web Password</b>"
                    "<br>"
                    "<input name='device_password' type='password' placeholder='Device Password' value='" + String(_config.device.password) + "'>"
                    "<p>"
                    "<b>Note:</b> username for authentication is <b>" HTTP_WEB_USER "</b>"
                    "<p>"
                    "<button name='save' type='submit' class='button greenbg'>Speichern</button>"
                    "</fieldset>"
                    "</form>"
                    "<p><form action='/config' method='get'><button>Configuration Menu</button></form><p>"
                    + _html_footer);
  });

  _WebServer.on("/config/wifi", []() {
    _last_request = millis();
    _WebServer.send(200, "text/html",
                    _html_header +
                    "<form method='get' action='/config'>"
                    "<fieldset>"
                    "<legend>"
                    "<b>&nbsp;WiFi&nbsp;</b>"
                    "</legend>"
                    "<b>SSID</b>"
                    "<br>"
                    "<input name='wifi_ssid' type='text' placeholder='WiFi SSID' value='" + String(_config.wifi.ssid) + "'>"
                    "<p>"
                    "<b>Password</b>"
                    "<br>"
                    "<input name='wifi_psk' type='password' placeholder='WiFi Password' value='" + String(_config.wifi.psk) + "'>"
                    "<p>"
                    "<button name='save' type='submit' class='button greenbg'>Speichern</button>"
                    "</fieldset>"
                    "</form>"
                    "<p><form action='/config' method='get'><button>Configuration Menu</button></form><p>"
                    + _html_footer);
  });

  _WebServer.on("/config/ntp", []() {
    _last_request = millis();
    _WebServer.send(200, "text/html",
                    _html_header +
                    "<form method='get' action='/config'>"
                    "<fieldset>"
                    "<legend>"
                    "<b>&nbsp;NTP&nbsp;</b>"
                    "</legend>"
                    "<b>Server</b>"
                    "<br>"
                    "<input name='ntp_server' type='text' placeholder='NTP server' value='" + String(_config.ntp.server) + "'>"
                    "<p>"
                    "<button name='save' type='submit' class='button greenbg'>Speichern</button>"
                    "</fieldset>"
                    "</form>"
                    "<p><form action='/config' method='get'><button>Configuration Menu</button></form><p>"
                    + _html_footer);
  });

  _WebServer.on("/config/mqtt", []() {
    _last_request = millis();
    _WebServer.send(200, "text/html",
                    _html_header +
                    "<form method='get' action='/config'>"
                    "<fieldset>"
                    "<legend>"
                    "<b>&nbsp;MQTT&nbsp;</b>"
                    "</legend>"
                    "<b>Server</b>"
                    "<br>"
                    "<input name='mqtt_server' type='text' placeholder='MQTT server' value='" + String(_config.mqtt.server) + "'>"
                    "<p>"
                    "<b>Port</b>"
                    "<br>"
                    "<input name='mqtt_port' type='text' placeholder='MQTT port' value='" + String(_config.mqtt.port) + "'>"
                    "<p>"
                    "<b>User (optional)</b>"
                    "<br>"
                    "<input name='mqtt_user' type='text' placeholder='MQTT user' value='" + String(_config.mqtt.user) + "'>"
                    "<p>"
                    "<b>Password (optional)</b>"
                    "<br>"
                    "<input name='mqtt_password' type='text' placeholder='MQTT password' value='" + String(_config.mqtt.password) + "'>"
                    "<p>"
                    "<b>ClientID</b>"
                    "<br>"
                    "<input name='mqtt_clientID' type='text' placeholder='MQTT ClientID' value='" + String(_config.mqtt.clientID) + "'>"
                    "<p>"
                    "<button name='save' type='submit' class='button greenbg'>Speichern</button>"
                    "</fieldset>"
                    "</form>"
                    "<p><form action='/config' method='get'><button>Configuration Menu</button></form><p>"
                    + _html_footer);
  });

  _WebServer.on("/config/ble", []() {
    _last_request = millis();
    _WebServer.send(200, "text/html",
                    _html_header +
                    "<form method='get' action='/config'>"
                    "<fieldset>"
                    "<legend>"
                    "<b>&nbsp;BLE&nbsp;</b>"
                    "</legend>"
                    "<b>Scan Time (" + BLE_SCAN_TIME_MIN + "s - " + BLE_SCAN_TIME_MAX + "s)</b>"
                    "<br>"
                    "<input name='ble_scan_time' type='text' placeholder='BLE scan time' value='" + String(_config.ble.scan_time) + "'>"
                    "<p>"
                    "<b>Pause Time (" + BLE_PAUSE_TIME_MIN + "s - " + BLE_PAUSE_TIME_MAX + "s)</b>"
                    "<br>"
                    "<input name='ble_pause_time' type='text' placeholder='BLE pause time' value='" + String(_config.ble.pause_time) + "'>"
                    "<p>"
                    "<button name='save' type='submit' class='button greenbg'>Speichern</button>"
                    "</fieldset>"
                    "</form>"
                    "<p><form action='/config' method='get'><button>Configuration Menu</button></form><p>"
                    + _html_footer);
  });

  _WebServer.on("/config/reset", []() {
    _last_request = millis();
    if (!StateCheck(STATE_CONFIGURING) && _config.device.password[0] && !_WebServer.authenticate(HTTP_WEB_USER, _config.device.password))
      return _WebServer.requestAuthentication();

    /*
       reset the config
    */
    memset(&_config, 0, sizeof(_config));
    ConfigSet(0, sizeof(CONFIG), &_config);

    _WebServer.send(200, "text/html",
                    _html_header +
                    "<div class='msg'>"
                    "Configuration was reset."
                    "<p>"
                    "Wait for the device to come up with an WiFi-AccessPoint, connect to it to configure the device."
                    "</div>"
                    + _html_footer);

    /*
        trigger reboot
    */
    StateChange(STATE_WAIT_BEFORE_REBOOTING);
  });

  _WebServer.on("/info", []() {
    _last_request = millis();
    if (_config.device.password[0] && !_WebServer.authenticate(HTTP_WEB_USER, _config.device.password))
      return _WebServer.requestAuthentication();

    _WebServer.send(200, "text/html",
                    _html_header +
                    "<div class='info'>"
                    "<table style='width:100%'"
                    "<tr>"
                    "<th>" __TITLE__ " Version</th>"
                    "<td>" GIT_VERSION "</td>"
                    "</tr>"
                    "<tr>"
                    "<th>Build Date</th>"
                    "<td>" __DATE__ " " __TIME__ "</td>"
                    "</tr>"
                    "<tr>"
                    "<th>Device Name</th>"
                    "<td>" + String(_config.device.name) + "</td>"
                    "</tr>"
                    "<tr><th></th><td>&nbsp;</td></tr>"
                    "<tr>"
                    "<th>WiFi SSID</th>"
                    "<td>" + WifiGetSSID() + "</td>"
                    "</tr>"
                    "<tr>"
                    "<th>WiFi RSSI</th>"
                    "<td>" + String(WIFI_RSSI_TO_QUALITY(WifiGetRSSI())) + "% (" + WifiGetRSSI() + "dBm)</td>"
                    "</tr>"
                    "<tr>"
                    "<th>WiFi MAC</th>"
                    "<td>" + WifiGetMacAddr() + "</td>"
                    "</tr>"
                    "<tr>"
                    "<th>WiFi IP Address</th>"
                    "<td>" + WifiGetIpAddr() + "</td>"
                    "</tr>"
                    "<tr><th></th><td>&nbsp;</td></tr>"
                    "<tr>"
                    "<th>NTP Server</th>"
                    "<td>" + _config.ntp.server + "</td>"
                    "</tr>"
                    "<tr><th></th><td>&nbsp;</td></tr>"
                    "<tr>"
                    "<th>MQTT Host</th>"
                    "<td>" + _config.mqtt.server + "</td>"
                    "</tr>"
                    "<tr>"
                    "<th>MQTT Port</th>"
                    "<td>" + _config.mqtt.port + "</td>"
                    "</tr>"
                    "<tr>"
                    "<th>MQTT User</th>"
                    "<td>" + _config.mqtt.user + "</td>"
                    "</tr>"
                    "<tr>"
                    "<th>MQTT Password</th>"
                    "<td>" + _config.mqtt.password + "</td>"
                    "</tr>"
                    "<tr>"
                    "<th>MQTT Client ID</th>"
                    "<td>" + _config.mqtt.clientID + "</td>"
                    "</tr>"
                    "<tr><th></th><td>&nbsp;</td></tr>"
                    "<tr>"
                    "<th>BLE Scan Time</th>"
                    "<td>" + _config.ble.scan_time + "</td>"
                    "</tr>"
                    "<tr>"
                    "<th>BLE Pause Time</th>"
                    "<td>" + _config.ble.pause_time + "</td>"
                    "</tr>"
                    "</table>"
                    "</div>"
                    "<p><form action='/' method='get'><button>Main Menu</button></form><p>"
                    + _html_footer);
  });

  _WebServer.on("/restart", []() {
    _last_request = millis();
    if (!StateCheck(STATE_CONFIGURING) && _config.device.password[0] && !_WebServer.authenticate(HTTP_WEB_USER, _config.device.password))
      return _WebServer.requestAuthentication();

    _WebServer.send(200, "text/html",
                    _html_header +
                    "<div class='msg'>"
                    "Device will restart now."
                    "</div>"
                    "<p><form action='/' method='get'><button>Main Menu</button></form><p>"
                    + _html_footer);

    /*
        trigger reboot
    */
    StateChange(STATE_WAIT_BEFORE_REBOOTING);
  });

  _WebServer.on("/ble", []() {
    _last_request = millis();
    _WebServer.send(200, "text/html",
                    _html_header +
                    BleScanListHTML() +
                    "<p><form action='/ble' method='get'><button class='button greenbg'>Reload</button></form><p>"
                    "<p><form action='/' method='get'><button>Main Menu</button></form><p>"
                    + _html_footer);
  });

  _WebServer.begin();
  _last_request = millis();
  LogMsg("HTTP: server started");
}

/*
**	handle incoming HTTP requests
*/
void HttpUpdate(void)
{
  _WebServer.handleClient();
}

/*
  return the time in seconds since the last HTTP request
*/
int HttpLastRequest(void)
{
  return (millis() - _last_request) / 1000;
}/**/
