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
#include "ble.h"
#include "mqtt.h"
#include "util.h"

/*
   BLE scan object
*/
static BLEScan *_BleScan = NULL;

/*
 * the last scan list as HTML
 */
static String _BleScanListHtml = "no scan so far";

/*
   Callback invoked when scanning has completed.
*/
static void BleScanComplete(BLEScanResults scanResults)
{
  time_t t = now();
  char time_buffer[25];

  LogMsg("BLE: finished scan -- Found %d device", scanResults.getCount());

  LogMsg("BLE: %-17.17s  %4.4s  %-20.20s  %s", "MAC", "RSSI", "Name","Vendor");
  LogMsg("BLE: ---------------------------------------------------------------------");

  strftime(time_buffer,sizeof(time_buffer),"%H:%M:%S  %d-%m-%Y",localtime(&t));
  _BleScanListHtml = "BLE Scan List: " + String(time_buffer) + "<p>";
  _BleScanListHtml += "<table class='blescanlist'>"
          "<tr>"
          "<th>MAC</th>"
          "<th>RSSI</th>"
          "<th>Name</th>"
          "<th>Vendor</th>"
          "</tr>";
  for (int i = 0; i < scanResults.getCount(); i++) {
    /*
       loop over the result list
    */
    BLEAdvertisedDevice device = scanResults.getDevice(i);

    /*
     * lookup the vendor
     */
    String MAC = String(device.getAddress().toString().c_str());
    MAC.toUpperCase();
    const byte *macaddr = StringToAddress((const char *) MAC.c_str(),6,false);
    const char *vendor = MacAddrLookup(macaddr);
      
    LogMsg("BLE: %-17.17s  %4.4s  %-20.20s  %s",
           MAC.c_str(),
           device.haveRSSI() ? String(device.getRSSI()).c_str() : "-",
           device.haveName() ? device.getName().c_str() : "-",
           (vendor) ? vendor : "-");

    /*
      publish the result
    */
    MqttPublish(MAC + "/last_seen", String(t));
    if (device.haveRSSI())
      MqttPublish(MAC + "/rssi", String(device.getRSSI()));
    if (device.haveName())
      MqttPublish(MAC + "/name", String(device.getName().c_str()));

    /*
       setup the list as HTML
    */
    _BleScanListHtml += "<tr>"
          "<td>" + MAC + "</td>"
          "<td>" + String(device.getRSSI()) + "</td>"
          "<td>" + String(device.getName().c_str()) + "</td>"
          "<td>" + String(vendor) + "</td>"
          "</tr>";
  }
  _BleScanListHtml += "</table>";

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
  if (_config_mode)
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
  _BleScan->start(BLE_SCAN_TIME, BleScanComplete, false);
}

/*
 * return the last BLE scan list as HTML
 */
String BleScanListHTML(void)
{
  return _BleScanListHtml;
}/**/
