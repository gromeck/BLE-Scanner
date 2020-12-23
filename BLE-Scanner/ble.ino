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
#include "ble.h"
#include "mqtt.h"
#include "macaddr.h"
#include "util.h"

/*
   BLE scan object
*/
static BLEScan *_BleScan = NULL;

/*
   this is our BLE device list implemented as a double-pointer list
*/
static int _ble_devices = 0;
static BLE_DEVICE *_ble_device_list_first = NULL;
static BLE_DEVICE *_ble_device_list_last = NULL;

/*
   setup
*/
void BleSetup(void)
{
  /*
   * check and correct the config
   */
  if (!_config.ble.scan_time)
    _config.ble.scan_time = BLE_SCAN_TIME_MAX;
  _config.ble.scan_time = min(max(_config.ble.scan_time,BLE_SCAN_TIME_MIN),BLE_SCAN_TIME_MAX);
  if (!_config.ble.pause_time)
    _config.ble.pause_time = BLE_PAUSE_TIME_MAX;
  _config.ble.pause_time = min(max(_config.ble.pause_time,BLE_PAUSE_TIME_MIN),BLE_PAUSE_TIME_MAX);

  /*
   * set the timeout values in the status table
   */
  LogMsg("BLE: setting up timout values in the status table");
  StateModifyTimeout(STATE_SCANNING, _config.ble.scan_time * 1000);
  StateModifyTimeout(STATE_PAUSING, _config.ble.pause_time * 1000);
}

/*
   Callback invoked when scanning has completed.
*/
static void BleScanComplete(BLEScanResults scanResults)
{
  LogMsg("BLE: finished scan -- Found %d device", scanResults.getCount());

  LogMsg("BLE: %-17.17s  %4.4s  %-20.20s  %s", "MAC", "RSSI", "Name", "Vendor");
  LogMsg("BLE: ---------------------------------------------------------------------");

  for (int i = 0; i < scanResults.getCount(); i++) {
    /*
       loop over the result list
    */
    BLEAdvertisedDevice device = scanResults.getDevice(i);

    /*
       lookup the vendor
    */
    String MAC = String(device.getAddress().toString().c_str());
    MAC.toUpperCase();
    const byte *macaddr = StringToAddress((const char *) MAC.c_str(), MAC_ADDR_LEN, false);
    const char *vendor = MacAddrLookup(macaddr);

    LogMsg("BLE: %-17.17s  %4.4s  %-20.20s  %s",
           MAC.c_str(),
           device.haveRSSI() ? String(device.getRSSI()).c_str() : "-",
           device.haveName() ? device.getName().c_str() : "-",
           (vendor) ? vendor : "-");

    /*
      publish the result
    */
    time_t t = now();
    MqttPublish(MAC + "/last_seen", String(t));
    if (device.haveRSSI())
      MqttPublish(MAC + "/rssi", String(device.getRSSI()));
    if (device.haveName())
      MqttPublish(MAC + "/name", String(device.getName().c_str()));
    if (vendor)
      MqttPublish(MAC + "/vendor", String(vendor));

    /*
       add the device to the list
    */
    BleDeviceListAdd(macaddr, device.getName().c_str(), device.getRSSI(), vendor);
  }

  /*
     trigger the state machine
  */
  StateChange(STATE_PAUSING);
}

/*
   initiate a BLE scan
*/
void BleStartScan(void)
{
  if (StateCheck(STATE_CONFIGURING))
    return;

  if (!_BleScan) {
    /*
       setup the bluetooth device
    */
    LogMsg("BLE: setting up bluetooth device");
    BLEDevice::deinit(true);
    BLEDevice::init(__TITLE__);

    /*
       init the scan object

       NOTE: actice scans use more power, but get results faster
    */
    LogMsg("BLE: setting up scan object");
    _BleScan = BLEDevice::getScan();
    _BleScan->setActiveScan(true);
  }

  /*
     initiate the scan

     NOTE: the so far known devices will be dropped
  */
  LogMsg("BLE: starting scan");
  _BleScan->start(_config.ble.scan_time - 1, BleScanComplete, false);
}

#if DBG
static void dumpBLEList(const char *title)
{
  /*
     dump the list
  */
  DbgMsg("BLE:%s: _ble_devices=%d", title, _ble_devices);
  DbgMsg("BLE:%s: _ble_device_list_first=%p", title, _ble_device_list_first);
  DbgMsg("BLE:%s: _ble_device_list_last=%p", title, _ble_device_list_last);
  for (BLE_DEVICE *device = _ble_device_list_first; device; device = device->next) {
    DbgMsg("BLE:%s: mac=%s  device=%p  next=%p  prev=%p  rssi=%d  name=%s  vendor=%s  last_seen=%lu",
           title,
           AddressToString(device->mac, MAC_ADDR_LEN, false),
           device, device->next, device->prev,
           device->rssi,
           device->name,
           (device->vendor) ? device->vendor : "-",
           device->last_seen);

  }
}
#endif

/*
   add a device to the device list
*/
bool BleDeviceListAdd(const byte * mac, const char *name, const int rssi, const char *vendor)
{
  BLE_DEVICE *device;

  /*
     scan our list to check if this device is already known
  */
  dumpBLEList("searchin");
  for (device = _ble_device_list_first; device; device = device->next) {
    if (!memcmp(device->mac, mac, MAC_ADDR_LEN)) {
      /*
         ok, this device was already seen
      */
      break;
    }
  }

  if (!device && _ble_devices >= BLE_DEVICE_LIST_MAX_LENGTH) {
    /*
       no device found, and the maximum number of devices is reached

       bring the first device in front
    */
    device = _ble_device_list_last;
  }

  if (device) {
    /*
       bring this device to the front
    */
    dumpBLEList("before bring to front");
    if (device->prev)
      device->prev->next = device->next;
    if (device->next)
      device->next->prev = device->prev;
    _ble_device_list_first->prev = device;
    if (_ble_device_list_last == device)
      _ble_device_list_last = device->prev;
    device->prev = NULL;
    device->next = _ble_device_list_first;
    _ble_device_list_first = device;
    dumpBLEList("after bring to front");
  }
  else {
    /*
       create a new device, at the front
    */
    if ((device = (BLE_DEVICE *) malloc(sizeof(BLE_DEVICE)))) {
      memset(device,0,sizeof(BLE_DEVICE));
      _ble_devices++;
      dumpBLEList("before new device");
      device->prev = NULL;
      device->next = _ble_device_list_first;
      _ble_device_list_first = device;
      if (device->next)
        device->next->prev = device;
      if (!_ble_device_list_last)
        _ble_device_list_last = device;
      dumpBLEList("after new device");
    }
  }

  /*
     copy the data into the device
  */
  if (device) {
    memcpy(device->mac, mac, MAC_ADDR_LEN);
    if (name && name[0])
      strncpy(device->name, name, BLE_DEVICE_NAME_LENGTH);
    device->rssi = rssi;
    device->vendor = vendor;
    device->last_seen = now();
  }

  return (device) ? true : false;
}

/*
   return the last BLE scan list as HTML
*/
String BleScanListHTML(void)
{
  if (!_ble_device_list_first)
    return String("no scan so far");

  String html = "BLE Scan List: " + String(TimeToString(now())) + "<p>";

  html += "<table class='blescanlist'>"
          "<tr>"
          "<th>MAC</th>"
          "<th>RSSI</th>"
          "<th>Name</th>"
          "<th>Vendor</th>"
          "<th>Last Seen</th>"
          "</tr>";

  for (BLE_DEVICE *device = _ble_device_list_first; device; device = device->next) {
    /*
       setup the list as HTML
    */
    html += "<tr>"
            "<td>" + String(AddressToString(device->mac, MAC_ADDR_LEN, false)) + "</td>"
            "<td>" + String(device->rssi) + "</td>"
            "<td>" + String((device->name[0]) ? device->name : "-") + "</td>"
            "<td>" + String((device->vendor) ? device->vendor : "-") + "</td>"
            "<td>" + String(TimeToString(device->last_seen)) + "</td>"
            "</tr>";
  }
  html += "</table>";

  return html;
}/**/
