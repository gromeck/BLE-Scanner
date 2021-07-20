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

#include <PubSubClient.h>
#include "config.h"

/*
   MQTT settings
*/
#define MQTT_PORT_DEFAULT         1883
#define MQTT_PORT_MIN             1024
#define MQTT_PORT_MAX             65535

#define MQTT_TOPIC_ANNOUNCE       "/status"
#define MQTT_TOPIC_CONTROL        "/control"
#define MQTT_TOPIC_DEVICE         "/device"

#define MQTT_PUBLISH_TIMEOUT_MIN  10            // seconds
#define MQTT_PUBLISH_TIMEOUT_MAX  (60 * 60)

/*
   if the MQTT connection failed, wait this time before retrying
*/
#define MQTT_WAIT_TO_RECONNECT  10

/*
   time for cyclic status update
*/
#define MQTT_STATUS_UPDATE_CYCLE  (5*60)

/*
   initialize the MQTT context
*/
void MqttSetup(void);

/*
   cyclic update of the MQTT context
*/
void MqttUpdate(void);

/*
   return true if we should publish all devices

   this will be done after a reconnect and from time to time
*/
bool MqttPublishAll(void);

/*
   publish the given message
*/
void MqttPublish(String suffix, String msg);

#endif

/**/
