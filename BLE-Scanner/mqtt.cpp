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
#include "ntp.h"

/*
   MQTT context
*/
static PubSubClient *_mqtt;
static String _topic_announce;
static String _topic_control;
static String _topic_device;
static time_t _last_reconnect = 0;
static time_t _last_status_update = 0;
static bool _publish_all = true;

/*
   initialize the MQTT context
*/
void MqttSetup(void)
{
  /*
     check and correct the config
  */
  if (!_config.mqtt.port)
    _config.mqtt.port = MQTT_PORT_DEFAULT;
  FIX_RANGE(_config.mqtt.port,MQTT_PORT_MIN, MQTT_PORT_MAX);
  _config.mqtt.publish_absence = _config.mqtt.publish_absence ? true : false;
  FIX_RANGE(_config.mqtt.publish_timeout, MQTT_PUBLISH_TIMEOUT_MIN, MQTT_PUBLISH_TIMEOUT_MAX);

  if (StateCheck(STATE_CONFIGURING))
    return;

  LogMsg("MQTT: setting up context");

  _mqtt = new PubSubClient(_wifiClient);
  _mqtt->setServer(_config.mqtt.server, _config.mqtt.port);

  _topic_announce = String(_config.mqtt.topicPrefix) + MQTT_TOPIC_ANNOUNCE;
  _topic_control = String(_config.mqtt.topicPrefix) + MQTT_TOPIC_CONTROL;
  _topic_device = String(_config.mqtt.topicPrefix) + MQTT_TOPIC_DEVICE;

#if DBG_MQTT
  DbgMsg("MQTT: _topic_announce: %s", _topic_announce.c_str());
  DbgMsg("MQTT: _topic_control: %s", _topic_control.c_str());
  DbgMsg("MQTT: _topic_device: %s", _topic_device.c_str());
#endif

  LogMsg("MQTT: context ready");
}

/*
   cyclic update of the MQTT context
*/
void MqttUpdate(void)
{
  if (StateCheck(STATE_CONFIGURING))
    return;

  if (!_mqtt->connected()) {
    if (now() > _last_reconnect + MQTT_WAIT_TO_RECONNECT) {
      /*
         connect the MQTT server
      */
      LogMsg("MQTT: reconnecting %s:%s@%s:%d width clientID %s ...", _config.mqtt.user, _config.mqtt.password, _config.mqtt.server, _config.mqtt.port, _config.mqtt.clientID);
      bool connect_status = _mqtt->connect(
                              _config.mqtt.clientID,
                              _config.mqtt.user,
                              _config.mqtt.password,
                              _topic_announce.c_str(),
                              2,  // willQoS
                              true,  // willRetain
                              "{ \"state\":\"disconnected\" }");

      if (connect_status) {
        /*
           we are connected
        */
        _publish_all = true;
        _mqtt->publish((_topic_announce + "/state").c_str(), "connected", true);

        // ... and resubscribe
        _mqtt->subscribe(_topic_control.c_str());
        _last_status_update = 0;
      }
      else {
        /*
           connection failed
        */
        LogMsg("MQTT: connection failed, rc=%d -- trying again in %d seconds", _mqtt->state(), MQTT_WAIT_TO_RECONNECT);
        _last_reconnect = now();
      }

#if DBG_MQTT
      DbgMsg("MQTT: connect_status=%d", connect_status);
#endif
    }
  }

  if (_mqtt->connected()) {
    /*
       we are connected, so we have to do the cyclic processing
    */
    _mqtt->loop();

    if (now() > _last_status_update + MQTT_STATUS_UPDATE_CYCLE) {
      /*
         it's time to publish our connection state
      */
      _last_status_update = now();
      _publish_all = true;
#if DBG_MQTT
      DbgMsg("MQTT: publishing connection state");
#endif

      String json = "{"
                    "\"state\":\"connected\","
                    "\"Time\":\"" + String(TimeToString(now())) + "\","
                    "\"Uptime\":\"" + String(TimeToString(NtpUptime())) + "\","
                    "\"UptimeSec\":" + String(NtpUptime()) + ","
                    "\"Wifi\":{"
                    "\"SSId\":\"" + WifiGetSSID() + "\","
                    "\"MacAddress\":\"" + WifiGetMacAddr() + "\","
                    "\"IpAddress\":\"" + WifiGetIpAddr() + "\","
                    "\"Channel\":" + WifiGetChannel() + ","
                    "\"RSSI\":" + WifiGetRSSI() + ","
                    "\"Signal\":\"" + String(WIFI_RSSI_TO_QUALITY(WifiGetRSSI())) + "%\""
                    "},"
                    "\"Version\":\"" + GIT_VERSION + "\""
                    "}";
#if DBG_MQTT
      DbgMsg("MQTT: %s", json.c_str());
#endif
      _mqtt->publish_P(_topic_announce.c_str(), json.c_str(), true);
    }
  }
}

/*
   return true if we should publish allo

   this will be done after a reconnect and from time to time
*/
bool MqttPublishAll(void)
{
  bool all = _publish_all;
  
  _publish_all = false;
  return all;
}

/*
   publish the given message
*/
void MqttPublish(String suffix, String msg)
{
  String topic = _topic_device + String("/") + suffix;

#if DBG_MQTT
  DbgMsg("MQTT: publishing: %s=%s", topic.c_str(), msg.c_str());
#endif

  _mqtt->publish_P(topic.c_str(), msg.c_str(), msg.length());
}/**/
