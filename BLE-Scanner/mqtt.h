/*
  BLE-Scanner

  (c) 2020 Christian.Lorenz@gromeck.de

  module to handle the MQTT stuff


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

#ifndef __MQTT_H__
#define __MQTT_H__ 1

#include "config.h"
#include <PubSubClient.h>

/*
   MQTT settings
*/
const char *MQTT_SERVER = "fhem.site";
const int   MQTT_PORT = 1883;
const char *MQTT_USER = "";
const char *MQTT_PASS = "";
const char *MQTT_CLIENTID = "blescanner2";

const char *MQTT_TOPIC_ANNOUNCE = "/status";
const char *MQTT_TOPIC_CONTROL = "/control";
const char *MQTT_TOPIC_DEVICE = "/device";


/*
   initialize the MQTT context
*/
void MqttSetup(void);

/*
   cyclic update of the MQTT context
*/
void MqttUpdate(void);

/*
   publish the given message
*/
void MqttPublish(String suffix, String msg);

#endif

/**/
