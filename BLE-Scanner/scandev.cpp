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
#include "ble-manufacturer.h"
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
bool ScanDevAdd(BLEAddress addr, const char *name, const uint16_t manufacturer_id, const int rssi, const bool has_battery)
{
  SCANDEV_T *device;
  int battery_level = 0;

  /*
     scan our list to check if this device is already known
  */
  DBG_SCANDEVLIST("searching");
  for (device = _scandev_first; device; device = device->next) {
    if (device->addr == addr) {
      /*
         ok, this device was already seen

         keep the battery level in mind
      */
      battery_level = device->battery_level;
      break;
    }
  }

  if (!device && _scandev_count >= SCANDEV_LIST_MAX_LENGTH) {
    /*
       no device found, and the maximum number of devices is reached

       pick the last device from the list -- it will get overwritten
    */
    device = _scandev_last;
    LogMsg("DEV: number of scanned devices in list: %d", _scandev_count);
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

    /*
       if this device slot was used from another device, we have to clean the record
    */
    if (device->addr != addr) {
      memset(device, 0, sizeof(SCANDEV_T));
    }
  }
  else {
    /*
       create a new device
    */
    if ((device = (SCANDEV_T *) malloc(sizeof(SCANDEV_T)))) {
      memset(device, 0, sizeof(SCANDEV_T));
    }
    LogMsg("DEV: number of scanned devices in list: %d", _scandev_count);
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
       copy the data into the device -- whenever the data changed
    */
    device->addr = addr;
    if (name && *name && strncmp(device->name, name, SCANDEV_NAME_LENGTH)) {
      /*
         device name changed
      */
      strncpy(device->name, name, SCANDEV_NAME_LENGTH);
      device->publish_name = true;
    }
    if (device->manufacturer_id != manufacturer_id) {
      /*
         manufacturer changed
      */
      device->manufacturer_id = manufacturer_id;
      device->manufacturer = BLEManufacturerLookup(manufacturer_id, "");
      device->publish_manufacturer = true;
    }
    if (device->has_battery != has_battery || device->battery_level != battery_level) {
      /*
         battery state or level changed
      */
      device->has_battery = has_battery;
      device->battery_level = battery_level;
      device->publish_battery = true;
    }
    if (device->rssi != rssi) {
      /*
         rssi changed
      */
      device->rssi = rssi;
      device->publish_rssi = true;
    }
    if (!device->present) {
      /*
         the prensence changed from absent to present
      */
      device->present = true;
      device->publish_presence = true;
    }

    /*
       last seen is always updated
    */
    device->last_seen = now();

    /*
       mark this device in general to publish
    */
    device->publish = true;

    DBG_SCANDEVLIST("after insert");
  }

  return (device) ? true : false;
}

/*
   check the battery level
*/
void ScanDevCheckBattery(SCANDEV_T *device)
{
  uint8_t battery_level = device->battery_level;

  if (BluetoothBatteryCheck(device->addr, &battery_level)) {
    /*
       we obviuosly had success reading the battery level ...
    */
    if (device->battery_level != battery_level) {
      /*
         ... and it has changed
      */
      device->battery_level = battery_level;
      device->publish_battery = true;
    }
  }

  /*
     even if the check failed, we will have to wait for the next cycle
  */
  device->last_battcheck = now();
}

/*
   publish all devices which are not yet published
*/
static void ScanDevPublishMQTT(SCANDEV_T *device, bool all)
{
  if (all || device->publish) {
    /*
       publish the device state
    */
    String json = "";
    String Addr = String(device->addr.toString().c_str());
    Addr.toUpperCase();
    Addr.replace(":", "-");

    if (all || device->publish_rssi) {
      if (json.length() > 0)
        json += ",";
      json += "\"RSSI\":" + String(device->rssi);
      device->publish_rssi = false;
    }
    if (all || device->publish_name) {
      if (json.length() > 0)
        json += ",";
      json += "\"Name\":\"" + String(device->name) + "\"";
      device->publish_name = false;
    }
    if (all || device->publish_manufacturer) {
      if (json.length() > 0)
        json += ",";
      json += "\"ManufacturerId\":\"" + String(BLEManufacturerIdHex(device->manufacturer_id)) + "\"";
      json += ",";
      json += "\"Manufacturer\":\"" + String(device->manufacturer) + "\"";
      device->publish_manufacturer = false;
    }
    if (all || device->publish_battery) {
      if (json.length() > 0)
        json += ",";
      json += "\"Battery\":" + String(device->has_battery ? 1 : 0);
      json += ",";
      json += "\"BatteryLevel\":" + String(device->battery_level);
      device->publish_battery = false;
    }
    if (all || json.length() > 0) {
      /*
         whenever we publish something, we will also publish the last_seen and the scanning device
      */
      json = "\"ScannerCID\":\"" + String(_config.mqtt.clientID) + "\"," + json;
      json = "\"Scanner\":\"" + String(_config.device.name) + "\"," + json;
      json = "\"last_seen\":" + String(device->last_seen) + "," + json;
    }
    if ((all || device->publish_presence || json.length() > 0) && (device->present || _config.mqtt.publish_absence)) {
      /*
         whenever we publish something, we will also publish the state
      */
      json = "\"state\":\"" + String((device->present) ? "present" : "absent") + "\"," + json;
      device->publish_presence = false;
    }
    if (json.length() > 0)
      MqttPublish(Addr, "{" + json + "}");

    device->publish = false;
    device->last_published = now();
  }
}

/*
   return the last BLE scan list as HTML
*/
void ScanDevListHTML(void (*callback)(const String& content))
{
  /*
     setup the list header
  */
  (*callback)("<p>Total scanned Bluetooth devices: " + String(_scandev_count) +
              " @ " + String(TimeToString(now())) + "</p>" +
              "<table class='btscanlist'>"
              "<tr>"
              "<th>State</th>"
              "<th>Address</th>"
              "<th>Name</th>"
              "<th>ManufacturerID</th>"
              "<th>Manufacturer</th>"
              "<th>RSSI [dbm]</th>"
              "<th>Distance [m]</th>"
              "<th>Last Seen</th>"
              "<th>Battery [%]</th>"
              "</tr>");

  if (_scandev_first) {
    for (SCANDEV_T *device = _scandev_first; device; device = device->next) {
      /*
         setup the list as HTML
      */
      (*callback)("<tr>"
                  "<td>" + String(device->present ? "✅" : "❌") + "</td>"
                  "<td>" + String(device->addr.toString().c_str()) + "</td>"
                  "<td>" + String((device->name[0]) ? device->name : "-") + "</td>"
                  "<td>" + String((device->manufacturer_id != BLE_MANUFACTURER_ID_UNKNOWN) ? BLEManufacturerIdHex(device->manufacturer_id) : "-") + "</td>"
                  "<td>" + String((device->manufacturer_id != BLE_MANUFACTURER_ID_UNKNOWN) ? device->manufacturer : "-") + "</td>"
                  "<td>" + String(device->rssi) + "</td>"
                  "<td>" + String(RSSI2METER(device->rssi)) + "</td>"
                  "<td>" + String(TimeToString(device->last_seen)) + "</td>"
                  "<td>" + ((device->has_battery) ? String(device->battery_level) : String("-")) + "</td>"
                  "</tr>");
    }
  }
  else {
    (*callback)("<tr>"
                "<td colspan=9>" + String("empty list") + "</td>"
                "</tr>");
  }
  (*callback)("</table>");
}

/*
   setup
*/
void ScanDevSetup(void)
{
}

/*
  update the scan device list

  publish all changed devices
*/
void ScanDevUpdate(void)
{
  SCANDEV_T *device;
  int absence_timeout = _config.bluetooth.absence_cycles * (_config.bluetooth.scan_time + _config.bluetooth.pause_time);
  static time_t _last = 0;
  bool all = MqttPublishAll();

  if (now() > _last) {
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
      if (device->present && now() - device->last_published > _config.mqtt.publish_timeout) {
        /*
           it's time to publish this device
        */
        device->publish = true;
      }

      if (device->has_battery && device->present && now() - device->last_battcheck > _config.bluetooth.battcheck_timeout) {
        /*
           time to check the battery
        */
        ScanDevCheckBattery(device);
      }

      /*
         publish the device
      */
      ScanDevPublishMQTT(device, all);
    }
    _last = now();
  }
}/**/
