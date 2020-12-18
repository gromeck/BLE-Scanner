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
     initialize the sub-systems
  */
  LogMsg("*** " __TITLE__ " - Version " GIT_VERSION " ***");
  StateSetup(STATE_PAUSING);
  LedSetup(LED_MODE_ON);
  ConfigSetup();
  WifiSetup();
  NtpSetup();
  HttpSetup();
  MqttSetup();

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
      LedSetup(LED_MODE_BLINK_FAST);
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
         we are now pausing
      */
      LedSetup(LED_MODE_OFF);
      break;

  }
}
