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
#include "state.h"
#include "util.h"
#include "bluetooth.h"
#include "scandev.h"
#include <esp_task_wdt.h>

void setup()
{
  /*
     setup serial communication
  */
  Serial.begin(115200);
  Serial.println();
  LogMsg("*** " __TITLE__ " - Version " GIT_VERSION " ***");

  /*
     initialize the watchdog

     the default timeout (5s) might be top short in debug mode
  */
  esp_task_wdt_init(30, true);

  /*
     initialize the basic sub-systems
  */
  LedSetup(LED_MODE_ON);
  StateSetup(STATE_SCANNING);

  if (!ConfigSetup())
    StateChange(STATE_CONFIGURING);

  if (StateCheck(STATE_CONFIGURING) || !WifiSetup()) {
    /*
       something went wrong -- enter configuration mode
    */
    LogMsg("SETUP: no WIFI connection -- entering configuration mode");
    StateSetup(STATE_CONFIGURING);
    WifiSetup();
  }

  /*
     setup the other sub-systems
  */
  HttpSetup();
  if (!StateCheck(STATE_CONFIGURING)) {
    ScanDevSetup();
    NtpSetup();
    BLEManufacturerSetup();
    MqttSetup();
    BluetoothSetup();
  }
}

void loop()
{
  /*
     do the cyclic updates of the sub systems
  */
  ConfigUpdate();
  LedUpdate();
  WifiUpdate();
  if (StateCheck(STATE_CONFIGURING)) {
    /*
        in configuration mode, we will only serve HTTP
    */
    HttpUpdate();

    /*
       if there is no activity via HTTP, we will reboot

       this is in case where the device has switched by its own
       into the configuration mode.
    */
    if (HttpLastRequest() > STATE_CONFIGURING_TIMEOUT) {
      LogMsg(__TITLE__ ": restarting device");
      StateChange(STATE_REBOOT);
    }
  }
  if (StateCheck(STATE_SCANNING) || StateCheck(STATE_PAUSING)) {
    /*
       in normal operation we work on all sub systems
    */
    HttpUpdate();
    NtpUpdate();
    MqttUpdate();
    BluetoothUpdate();
    ScanDevUpdate();
  }

  /*
     what to do?
  */
  switch (StateUpdate()) {
    case STATE_SCANNING:
      /*
         start the scanner
      */
      LogMsg(__TITLE__ ": scanning");
      LedSetup(LED_MODE_BLINK_SLOW);
      BluetoothScanStart();
      break;
    case STATE_PAUSING:
      /*
         we are now pausing
      */
      LogMsg(__TITLE__ ": pausing");
      LedSetup(LED_MODE_BLINK_SLOW);
      BluetoothScanStop();
      break;
    case STATE_CONFIGURING:
      /*
         time to configure the device
      */
      LogMsg(__TITLE__ ": entering configuration mode -- rebooting in %d seconds if no configuration activity is registered",
             STATE_CONFIGURING_TIMEOUT - HttpLastRequest());
      LedSetup(LED_MODE_BLINK_FAST);
      break;
    case STATE_REBOOT:
      /*
         time to boot
      */
      LogMsg(__TITLE__ ": restarting the device");
      LedSetup(LED_MODE_OFF);
      ESP.restart();
      break;
  }
}
