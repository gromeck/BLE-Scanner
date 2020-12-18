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
**  Initialize the Ethernet server library
**  with the IP address and port you want to use
*/
static WebServer _WebServer(80);

static const char *HTML_HEADER PROGMEM =
  "<!DOCTYPE html><html>"
  "<head>"
  "<meta charset='utf-8'>"
  "<title>" __TITLE__ "</title>"
  "<link href='/styles.css' rel='stylesheet' type='text/css'/>"
  "</head>"
  "<body>"
  "<div class=content>"
  "<div class=header>"
  "<h1>" __TITLE__ "</h1>"
  "<h2>" "socket1.site" "</h2></div>"
  "</div>"
  ;

static const char *HTML_FOOTER PROGMEM =
  "</div>"
  "<hr>"
  "<a href='https://github.com/gromeck/BLE-Scanner' target='_blank' style='color:#aaa;'>" __TITLE__ " Version: " GIT_VERSION "</a>"
  "</body>"
  "</html>";

static const char *HTML_CSS PROGMEM =
  "html, body { background:#ffffff; }"
  "body { margin:0; padding:0; font-familiy:'sans-serif'; color:#202020; text-align:center; }"
  "button { border: 0; border-radius: 0.3rem; background: #1fa3ec; color: #ffffff; line-height: 2.4rem; font-size: 1.2rem; width: 100%; -webkit-transition-duration: 0.4s; transition-duration: 0.4s; cursor: pointer; }"
  ".content { text-align:left; display:inline-block; color:#000000; min-width:340px; }"
  ".header { text-align:center; }"
  ".greenbg { background: #47c266 }"
  ;

/*
   setup the webserver
*/
void HttpSetup(void)
{
  LogMsg("HTTP: setting up HTTP server");

  _WebServer.onNotFound([]() {
    String html = HTML_HEADER;
    html +=
      "<form action='/config' method='get'><button>Configuration</button></form><p>"
      "<form action='/info' method='get'><button>Information</button></form><p>";
    html += HTML_FOOTER;

    _WebServer.send(200, "text/html", html);
  });

  _WebServer.on("/styles.css", []() {
    _WebServer.send(200, "text/css", HTML_CSS);
  });


  _WebServer.on("/config", []() {
    String html = HTML_HEADER;
    html +=
      "<form action='/config/wifi' method='get'><button>Configure WiFi</button></form><p>"
      "<form action='/config/ntp' method='get'><button>Configure NTP</button></form><p>"
      "<form action='/config/mqtt' method='get'><button>Configure MQTT</button></form><p>"
      "<form action='/config/http' method='get'><button>Configure HTTP</button></form><p>"
      "<form action='/config/ble' method='get'><button>Configure BLE Scan</button></form><p>"
      "<p><form action='.' method='get'><button>Main Menu</button></form><p>";
    html += HTML_FOOTER;
    _WebServer.send(200, "text/html", html);
  });

  _WebServer.on("/config/wifi", []() {
    String html = HTML_HEADER;
    html +=
      "<fieldset>"
      "<legend>"
      "<b>&nbsp;Sonstige Einstellungen&nbsp;</b>"
      "</legend>"
      "<form method='get' action='/config'>"
      "<b>Passwort für Web Oberfläche</b>"
      "<input type='checkbox'>"
      "<br>"
      "<input id='wp' type='password' placeholder='Passwort für Web Oberfläche' value='****' name='wp'>"
      "<br><br>"
      "<input id='b1' type='checkbox' checked='' name='b1'>"
      "<b>MQTT aktivieren</b>"
      "<br><br>"
      "<b>Name [friendly name] 1</b> (WLAN-Switch)<br>"
      "<input id='a0' placeholder='WLAN-Switch' value='socket1.site' name='a0'>"
      "</p>"
      "</fieldset>"
      "<br>"
      "<button name='save' type='submit' class='button greenbg'>Speichern</button>"
      "</form>"
      "</fieldset>";
    html +=
      "<p><form action='.' method='get'><button>Main Menu</button></form><p>";
    html += HTML_FOOTER;
    _WebServer.send(200, "text/html", html);
  });

  _WebServer.on("/info", []() {
    String html = HTML_HEADER;
    html += "<div class='info'>"
            "<table style='width:100%'"

            "<tr>"
            "<th>" __TITLE__ " Version</th>"
            "<td>" GIT_VERSION "</td>"
            "</tr>"

            "<tr>"
            "<th>Build Date</th>"
            "<td>" __DATE__ " " __TIME__ "</td>"
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
            "<td>" + "TODO" + "</td>"
            "</tr>"

            "<tr><th></th><td>&nbsp;</td></tr>"

            "<tr>"
            "<th>MQTT Host</th>"
            "<td>" + "TODO" + "</td>"
            "</tr>"

            "<tr>"
            "<th>MQTT Port</th>"
            "<td>" + "TODO" + "</td>"
            "</tr>"

            "<tr>"
            "<th>MQTT User</th>"
            "<td>" + "TODO" + "</td>"
            "</tr>"

            "<tr>"
            "<th>MQTT Password</th>"
            "<td>" + "TODO" + "</td>"
            "</tr>"

            "<tr>"
            "<th>MQTT Client ID</th>"
            "<td>" + "TODO" + "</td>"
            "</tr>"

            "<tr>"
            "<th>MQTT Topic</th>"
            "<td>" + "TODO" + "</td>"
            "</tr>"

            "<tr><th></th><td>&nbsp;</td></tr>"

            "<tr>"
            "<th>BLE Scan Time</th>"
            "<th>" + "TODO" + "</th>"
            "</tr>"

            "<tr>"
            "<th>BLE Pause Time</th>"
            "<td>" + "TODO" + "</td>"
            "</tr>"

            "</table>";

    html +=
      "<p><form action='.' method='get'><button>Main Menu</button></form><p>";
    html += HTML_FOOTER;
    _WebServer.send(200, "text/html", html);
  });

  _WebServer.begin();
  LogMsg("HTTP: server started");
}

/*
**	handle incoming HTTP requests
*/
void HttpUpdate(void)
{
  _WebServer.handleClient();
}/**/
