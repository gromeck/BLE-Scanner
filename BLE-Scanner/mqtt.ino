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

#include "config.h"
#include "mqtt.h"
#include "wifi.h"
#include "util.h"

/*
   MQTT context
*/
static PubSubClient *_mqtt;
static String _topic_announce;
static String _topic_control;
static String _topic_device;

/*
   initialize the MQTT context
*/
void MqttSetup(void)
{
  if (_config_mode)
    return;

  LogMsg("MQTT: setting up context");

  _mqtt = new PubSubClient(_wifiClient);
  _mqtt->setServer(MQTT_SERVER, MQTT_PORT);

  _topic_announce = WifiGetSSID() + "/" + MQTT_CLIENTID + MQTT_TOPIC_ANNOUNCE;
  _topic_control = WifiGetSSID() + "/" + MQTT_CLIENTID + MQTT_TOPIC_CONTROL;
  _topic_device = WifiGetSSID() + "/" + MQTT_CLIENTID + MQTT_TOPIC_DEVICE;

  DbgMsg("MQTT: _topic_announce: %s", _topic_announce.c_str());
  DbgMsg("MQTT: _topic_control: %s", _topic_control.c_str());
  DbgMsg("MQTT: _topic_device: %s", _topic_device.c_str());

  LogMsg("MQTT: context ready");
}

/*
   cyclic update of the MQTT context
*/
void MqttUpdate(void)
{
  if (_config_mode)
    return;

  if (!_mqtt->connected()) {
    /*
       connect the MQTT server
    */
    LogMsg("MQTT: reconnecting %s:%s@%s:%d width clientID %s ...", MQTT_USER, MQTT_PASS, MQTT_SERVER, MQTT_PORT, MQTT_CLIENTID);
    bool connect_status = _mqtt->connect(
                            MQTT_CLIENTID,
                            MQTT_USER,
                            MQTT_PASS,
                            _topic_announce.c_str(),
                            2,  // willQoS
                            true,  // willRetain
                            "{ \"state\":\"disconnected\" }");

    DbgMsg("MQTT: connect_status=%d", connect_status);

    if (connect_status) {
      DbgMsg("MQTT: connected");
      /*
         publish our connection state
      */
      DbgMsg("MQTT: publishing connection state");
      _mqtt->publish((_topic_announce + "/state").c_str(), "connected", true);
      _mqtt->publish((_topic_announce + "/ssid").c_str(), WifiGetSSID().c_str(), true);
      _mqtt->publish((_topic_announce + "/ipaddr").c_str(), WifiGetIpAddr().c_str(), true);

      // ... and resubscribe
      _mqtt->subscribe(_topic_control.c_str());
    }
    else {
      /*
         connection failed
      */
      LogMsg("MQTT: connection failed, rc=%d -- trying again in 5 seconds", _mqtt->state());
      delay(5000);
    }
  }
  else
    _mqtt->loop();
}

/*
   publish the given message
*/
void MqttPublish(String suffix, String msg)
{
  String topic = _topic_device + String("/") + suffix;

  DbgMsg("MQTT: publishing: %s=%s", topic.c_str(), msg.c_str());

  _mqtt->publish(topic.c_str(), msg.c_str(), msg.length());
}/**/
