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
       copy the data into the device
    */
    device->addr = addr;
    if (name && name[0])
      strncpy(device->name, name, SCANDEV_NAME_LENGTH);
    device->manufacturer_id = manufacturer_id;
    device->manufacturer = BLEManufacturerLookup(manufacturer_id, "");
    device->has_battery = has_battery;
    device->battery_level = battery_level;
    device->rssi = rssi;
    device->last_seen = now();
    if (!device->present) {
      /*
         the prensence changed from absent to present
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
static void ScanDevPublishMQTT(SCANDEV_T *device)
{
  /*
     publish the device state
  */
  String Addr = String(device->addr.toString().c_str());
  Addr.toUpperCase();

  MqttPublish(Addr + "/last_seen", String(device->last_seen));
  if (device->present || _config.bluetooth.publish_absence)
    MqttPublish(Addr + "/presence", (device->present) ? "present" : "absent");
  MqttPublish(Addr + "/rssi", String(device->rssi));
  MqttPublish(Addr + "/name", String(device->name));
  MqttPublish(Addr + "/manufacturer_id", String(BLEManufacturerIdHex(device->manufacturer_id)));
  MqttPublish(Addr + "/manufacturer", String(device->manufacturer));
  MqttPublish(Addr + "/battery", String(device->has_battery ? 1 : 0));
  MqttPublish(Addr + "/battery_level", String(device->battery_level));
  device->publish = false;
  device->last_published = now();
}

/*
   check the battery level
*/
void ScanDevCheckBattery(SCANDEV_T *device)
{
  BluetoothBatteryCheck(device->addr,&device->battery_level);
  device->last_battcheck = now();
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
      if (device->present && now() - device->last_published > _config.bluetooth.publish_timeout) {
        /*
           it's time to publish this device
        */
        device->publish = true;
      }

      if (device->publish)
        ScanDevPublishMQTT(device);

      if (device->has_battery && device->present && now() - device->last_battcheck > _config.bluetooth.battcheck_timeout)
        ScanDevCheckBattery(device);
    }
    _last = now();
  }
}/**/
