/*
  BLE-Scanner

  (c) 2020 Christian.Lorenz@gromeck.de

  main module


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
#include "led.h"
#include "wifi.h"
#include "ntp.h"
#include "http.h"
#include "mqtt.h"
#include "ble.h"
#include "state.h"
#include "util.h"
#include "git-version.h"

void setup()
{
  /*
     setup serial communication
  */
  Serial.begin(115200);
  Serial.println();

  /*
     initialize the basic sub-systems
  */
  LogMsg("*** " __TITLE__ " - Version " GIT_VERSION " ***");
  LedSetup(LED_MODE_ON);
  ConfigSetup();
  if (!WifiSetup()) {
    /*
       something wen't wrong -- enter configuration mode
    */
    LogMsg("SETUP: no WIFI connection -- entering configuration mode");
    StateSetup(STATE_CONFIGURING);
    WifiSetup();
  }
  else {
    /*
       start in operational mode
    */
    StateSetup(STATE_PAUSING);
  }

  /*
     setup the other sub-systems
  */
  NtpSetup();
  HttpSetup();
  MqttSetup();
  BleSetup();

  delay(1000);
}

void loop()
{
  /*
     do the cyclic updates of the sub systems
  */
  ConfigUpdate();
  LedUpdate();
  WifiUpdate();
  NtpUpdate();
  HttpUpdate();
  MqttUpdate();

  /*
     what to do?
  */
  switch (StateUpdate()) {
    case STATE_SCANNING:
      /*
         start the scanner
      */
      LedSetup(LED_MODE_BLINK_SLOW);
      BleStartScan();
      break;
    case STATE_PAUSING:
      /*
         we are now pausing
      */
      LedSetup(LED_MODE_BLINK_SLOW);
      break;
    case STATE_CONFIGURING:
      /*
         time to configure the device
      */
      LedSetup(LED_MODE_BLINK_FAST);

      /*
         if there is no activity via HTTP, we will reboot

         this is in case where the device has switched by its own
         into the configuration mode.
      */
      if (HttpLastRequest() > STATE_CONFIGURING_TIMEOUT) {
        LogMsg("LOOP: restarting hte device");
        StateChange(STATE_REBOOT);
      }
      break;
    case STATE_REBOOT:
      /*
         time to boot
      */
      LogMsg("LOOP: restarting hte device");
      LedSetup(LED_MODE_OFF);
      ESP.restart();
      break;
  }
}
