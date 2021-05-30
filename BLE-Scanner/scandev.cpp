/*
  BLE-Scanner

  (c) 2020 Christian.Lorenz@gromeck.de

  module to handle the BLE scan


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
#include "state.h"
#include "bluetooth.h"
#include "mqtt.h"
#include "macaddr.h"
#include "util.h"
#include "scandev.h"

static int _scandev_count = 0;
static SCANDEV_T *_scandev_first = NULL;
static SCANDEV_T *_scandev_last = NULL;

#if DBG_SCANDEV
/*
   dump the bluetoot device list
*/
static void dumpScanDevList(const char *title)
{
  /*
     dump the list
  */
  DbgMsg("SCANDEV:%s: _scandev_count=%d", title, _scandev_count);
  DbgMsg("SCANDEV:%s: _scandev_first=%p", title, _scandev_first);
  DbgMsg("SCANDEV:%s: _scandev_last=%p", title, _scandev_last);
  for (SCANDEV_T *device = _scandev_first; device; device = device->next) {
    DbgMsg("SCANDEV:%s: devtype=%d  mac=%s  device=%p  next=%p  prev=%p  rssi=%d  name=%s  vendor=%s  last_seen=%lu",
           title,
           device->devtype,
           AddressToString(device->mac, MAC_ADDR_LEN, false, ':'),
           device, device->next, device->prev,
           device->rssi,
           device->name,
           device->vendor,
           device->last_seen);

  }
}
#define DBG_SCANDEVLIST(title)  dumpScanDevList(title)
#else
#define DBG_SCANDEVLIST(title)  {}
#endif

/*
   add a device to the device list
*/
bool ScanDevAdd(SCANDEV_TYPE_T devtype, const byte * mac, const char *name, const int rssi, const int cod)
{
  SCANDEV_T *device;

  /*
     scan our list to check if this device is already known
  */
  DBG_SCANDEVLIST("searching");
  for (device = _scandev_first; device; device = device->next) {
    if (!memcmp(device->mac, mac, MAC_ADDR_LEN)) {
      /*
         ok, this device was already seen
      */
      break;
    }
  }

  if (!device && _scandev_count >= SCANDEV_LIST_MAX_LENGTH) {
    /*
       no device found, and the maximum number of devices is reached

       pick the last device from the list -- it will get overwritten
    */
    device = _scandev_last;
  }

  if (device) {
    /*
       de-list this device
    */
    DBG_SCANDEVLIST("before de-listing");
    if (device->prev)
      device->prev->next = device->next;
    if (device->next)
      device->next->prev = device->prev;
    if (_scandev_first == device)
      _scandev_first = device->next;
    if (_scandev_last == device)
      _scandev_last = device->prev;
    device->prev = NULL;
    device->next = NULL;
    _scandev_count--;
    DBG_SCANDEVLIST("after de-listing");
  }
  else {
    /*
       create a new device
    */
    if ((device = (SCANDEV_T *) malloc(sizeof(SCANDEV_T)))) {
      memset(device, 0, sizeof(SCANDEV_T));
    }
  }

  if (device) {
    /*
       put this device at the begining of the list
    */
    DBG_SCANDEVLIST("before insert");

    if (_scandev_first)
      _scandev_first->prev = device;
    device->next = _scandev_first;
    _scandev_first = device;
    if (!_scandev_last)
      _scandev_last = device;
    _scandev_count++;

    /*
       copy the data into the device
    */
    memcpy(device->mac, mac, MAC_ADDR_LEN);
    if (name && name[0])
      strncpy(device->name, name, SCANDEV_NAME_LENGTH);
    device->devtype = devtype;
    device->rssi = rssi;
    device->cod = cod;
    device->vendor = MacAddrLookup(mac,"");
    device->last_seen = now();
    if (!device->present) {
      /*
       * the prensence changed from absent to present
       */
      device->present = true;
      device->publish = true;
    }
    DBG_SCANDEVLIST("after insert");
  }

  return (device) ? true : false;
}

/*
   publish all devices which are not yet published
*/
static void ScanDevPublishMQTT(void)
{
  SCANDEV_T *device;
  int absence_timeout = _config.bluetooth.absence_cycles * (_config.bluetooth.btc_scan_time + _config.bluetooth.ble_scan_time + _config.bluetooth.pause_time);

  /*
     scan our list to check if this device is already known
  */
  for (device = _scandev_first; device; device = device->next) {
    /*
       set the device absent, if it was too long unseen
    */
    if (device->present && now() - device->last_seen > absence_timeout) {
      /*
         the device is absent
      */
      device->present = false;
      device->publish = true;
    }
    if (device->present && now() - device->last_published > _config.bluetooth.publish_timeout) {
      /*
         it's time to publish this device
      */
      device->publish = true;
    }

    if (device->publish) {
      /*
         publish the device state
      */
      String MAC = String(AddressToString(device->mac, MAC_ADDR_LEN, false, ':'));
      MAC.toUpperCase();

      MqttPublish(MAC + "/last_seen", String(device->last_seen));
      if (device->present || _config.bluetooth.publish_absence)
        MqttPublish(MAC + "/presence", (device->present) ? "present" : "absent");
      MqttPublish(MAC + "/rssi", String(device->rssi));
      MqttPublish(MAC + "/name", String(device->name));
      MqttPublish(MAC + "/vendor", String(device->vendor));
      device->publish = false;
      device->last_published = now();
    }
  }
}

/*
   return the last BLE scan list as HTML
*/
String ScanDevListHTML(void)
{
  String html = "Bluetooth Scan List: " + String(TimeToString(now())) + "<p>";

  html += "<table class='btscanlist'>"
          "<tr>"
          "<th>Type</th>"
          "<th>MAC</th>"
          "<th>RSSI [dbm]</th>"
          "<th>Distance [m]</th>"
          "<th>Name</th>"
          "<th>Vendor</th>"
          "<th>Last Seen</th>"
          "<th>State</th>"
          "</tr>";

  if (_scandev_first) {
    for (SCANDEV_T *device = _scandev_first; device; device = device->next) {
      /*
         setup the list as HTML
      */
      html += "<tr>"
              "<td>" + String((device->devtype == SCANDEV_TYPE_BLE) ? "BLE" : "BTC") + "</td>"
              "<td>" + String(AddressToString(device->mac, MAC_ADDR_LEN, false, ':')) + "</td>"
              "<td>" + String(device->rssi) + "</td>"
              "<td>" + String(RSSI2METER(device->rssi)) + "</td>"
              "<td>" + String((device->name[0]) ? device->name : "-") + "</td>"
              "<td>" + String(device->vendor) + "</td>"
              "<td>" + String(TimeToString(device->last_seen)) + "</td>"
              "<td>" + String(device->present ? "present" : "absent") + "</td>"
              "</tr>";
    }
  }
  else {
    html += "<tr>"
            "<td colspan=8>" + String("empty list") + "</td>"
            "</tr>";
  }

  html += "<tr>"
          "<th colspan=8>Total of " + String(_scandev_count) + " scanned bluetooth devices.</th>"
          "</tr>"
          "</table>";

  return html;
}


/*
   setup
*/
void ScanDevSetup(void)
{
}

/*
  update the scan device list

  publish all changed devices -- but only once a second
*/
void ScanDevUpdate(void)
{
  static unsigned long _last_publish_mqtt = 0;

  if (now() - _last_publish_mqtt > 0) {
    ScanDevPublishMQTT();
    _last_publish_mqtt = now();
  }
}/**/
