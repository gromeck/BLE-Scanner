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

#include <esp_bt.h>
#include <esp_bt_main.h>
#include <esp_gap_bt_api.h>
#include <esp_gap_ble_api.h>
#include <esp_bt_device.h>

static int _bluetooth_mode = BLUETOOTH_MODE_IDLE;

static int _bluetooth_devices = 0;
static BLUETOOTH_DEVICE *_bluetooth_device_list_first = NULL;
static BLUETOOTH_DEVICE *_bluetooth_device_list_last = NULL;

#if DBG
static void dumpBluetoothDeviceList(const char *title)
{
  /*
     dump the list
  */
  DbgMsg("BLE:%s: _bluetooth_devices=%d", title, _bluetooth_devices);
  DbgMsg("BLE:%s: _bluetooth_device_list_first=%p", title, _bluetooth_device_list_first);
  DbgMsg("BLE:%s: _bluetooth_device_list_last=%p", title, _bluetooth_device_list_last);
  for (BLUETOOTH_DEVICE *device = _bluetooth_device_list_first; device; device = device->next) {
    DbgMsg("BLE:%s: devtype=%d  mac=%s  device=%p  next=%p  prev=%p  rssi=%d  name=%s  vendor=%s  last_seen=%lu",
           title,
           device->devtype,
           AddressToString(device->mac, MAC_ADDR_LEN, false, ':'),
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
bool BlootoothDeviceListAdd(bluetooth_devtype devtype, const byte * mac, const char *name, const int rssi, const int cod)
{
  BLUETOOTH_DEVICE *device;

  /*
     scan our list to check if this device is already known
  */
  dumpBluetoothDeviceList("searching");
  for (device = _bluetooth_device_list_first; device; device = device->next) {
    if (!memcmp(device->mac, mac, MAC_ADDR_LEN)) {
      /*
         ok, this device was already seen
      */
      break;
    }
  }

  if (!device && _bluetooth_devices >= BLUETOOTH_DEVICE_LIST_MAX_LENGTH) {
    /*
       no device found, and the maximum number of devices is reached

       pick the last device from the list -- it will get overwritten
    */
    device = _bluetooth_device_list_last;
  }

  if (device) {
    /*
       de-list this device
    */
    dumpBluetoothDeviceList("before de-listing");
    if (device->prev)
      device->prev->next = device->next;
    if (device->next)
      device->next->prev = device->prev;
    if (_bluetooth_device_list_first == device)
      _bluetooth_device_list_first = device->next;
    if (_bluetooth_device_list_last == device)
      _bluetooth_device_list_last = device->prev;
    device->prev = NULL;
    device->next = NULL;
    _bluetooth_devices--;
    dumpBluetoothDeviceList("after de-listing");
  }
  else {
    /*
       create a new device
    */
    if ((device = (BLUETOOTH_DEVICE *) malloc(sizeof(BLUETOOTH_DEVICE)))) {
      memset(device, 0, sizeof(BLUETOOTH_DEVICE));
    }
  }

  if (device) {
    /*
       put this device at the begining of the list
    */
    dumpBluetoothDeviceList("before insert");

    if (_bluetooth_device_list_first)
      _bluetooth_device_list_first->prev = device;
    device->next = _bluetooth_device_list_first;
    _bluetooth_device_list_first = device;
    if (!_bluetooth_device_list_last)
      _bluetooth_device_list_last = device;
    _bluetooth_devices++;

    /*
       copy the data into the device
    */
    memcpy(device->mac, mac, MAC_ADDR_LEN);
    if (name && name[0])
      strncpy(device->name, name, BLUETOOTH_DEVICE_NAME_LENGTH);
    device->devtype = devtype;
    device->rssi = rssi;
    device->cod = cod;
    device->vendor = MacAddrLookup(mac);
    device->last_seen = now();
    device->present = true;

    dumpBluetoothDeviceList("after insert");
  }

  return (device) ? true : false;
}

/*
   publish all devices which are not yet published
*/
static void blootoothPublishMQTT(void)
{
  BLUETOOTH_DEVICE *device;
  int absence_timeout = _config.bluetooth.absence_cycles * (_config.bluetooth.btc_scan_time + _config.bluetooth.ble_scan_time + _config.bluetooth.pause_time);

  /*
     scan our list to check if this device is already known
  */
  for (device = _bluetooth_device_list_first; device; device = device->next) {
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
      MqttPublish(MAC + "/vendor", (device->vendor) ? String(device->vendor) : "");
      device->publish = false;
    }
  }
}

/*
   return the last BLE scan list as HTML
*/
String BluetoothScanListHTML(void)
{
  String html = "Bluetooth Scan List: " + String(TimeToString(now())) + "<p>";

  html += "<table class='btscanlist'>"
          "<tr>"
          "<th>Type</th>"
          "<th>MAC</th>"
          "<th>RSSI</th>"
          "<th>Name</th>"
          "<th>Vendor</th>"
          "<th>Last Seen</th>"
          "<th>State</th>"
          "</tr>";

  if (_bluetooth_device_list_first) {
    for (BLUETOOTH_DEVICE *device = _bluetooth_device_list_first; device; device = device->next) {
      /*
         setup the list as HTML
      */
      html += "<tr>"
              "<td>" + String((device->devtype == BLUETOOTH_DEVTYPE_BLE) ? "BLE" : "BTC") + "</td>"
              "<td>" + String(AddressToString(device->mac, MAC_ADDR_LEN, false, ':')) + "</td>"
              "<td>" + String(device->rssi) + "</td>"
              "<td>" + String((device->name[0]) ? device->name : "-") + "</td>"
              "<td>" + String((device->vendor) ? device->vendor : "-") + "</td>"
              "<td>" + String(TimeToString(device->last_seen)) + "</td>"
              "<td>" + String(device->present ? "present" : "absent") + "</td>"
              "</tr>";
    }
  }
  else {
    html += "<tr>"
            "<td colspan=7>" + String("empty list") + "</td>"
            "</tr>";
  }
  
  html += "<tr>"
          "<th colspan=7>Total of " + String(_bluetooth_devices) + " scanned bluetooth devices.</th>"
          "</tr>"
          "</table>";

  return html;
}

static void btc_scan_callback(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
  char name[20] = "";
  byte *macaddr = NULL;
  int rssi = 0, cod = 0;

  //DbgMsg("BLUETOOTH:BTC:bluetooth_scan_callback: event=%d", event);
  switch (event) {
    case ESP_BT_GAP_DISC_RES_EVT:
      LogMsg("BLUETOOTH:BTC:bluetooth_scan_callback:ESP_BT_GAP_DISC_RES_EVT: %s: properties: %d",
             AddressToString(param->disc_res.bda, 6, false, ':'), param->disc_res.num_prop);

      /*
         init the information about the device
      */
      macaddr = (byte *) param->disc_res.bda;

      /*
         see what the properties contain
      */
      for (int n = 0; n < param->disc_res.num_prop; n++) {
        LogMsg("BLUETOOTH:BTC:bluetooth_scan_callback:ESP_BT_GAP_DISC_RES_EVT: %s: prop[%d]: type=%d",
               AddressToString(macaddr, 6, false, ':'), n, param->disc_res.prop[n].type);
        dump("BLUETOOTH:BTC:bluetooth_scan_callback:ESP_BT_GAP_DISC_RES_EVT: prop", param->disc_res.prop[n].val, param->disc_res.prop[n].len);
        switch (param->disc_res.prop[n].type) {
          case ESP_BT_GAP_DEV_PROP_BDNAME:
            /*
               device name
            */
            LogMsg("BLUETOOTH:BTC:bluetooth_scan_callback:ESP_BT_GAP_DISC_RES_EVT: %s  device name: %s",
                   AddressToString(macaddr, 6, false, ':'), (char *) param->disc_res.prop[n].val);
            break;
          case ESP_BT_GAP_DEV_PROP_COD:
            /*
               device class
            */
            {
              int major = (*((uint32_t *) param->disc_res.prop[n].val) & ESP_BT_COD_MAJOR_DEV_BIT_MASK) >> ESP_BT_COD_MAJOR_DEV_BIT_OFFSET;
              int minor = (*((uint32_t *) param->disc_res.prop[n].val) & ESP_BT_COD_MINOR_DEV_BIT_MASK) >> ESP_BT_COD_MINOR_DEV_BIT_OFFSET;
              int ftype = (*((uint32_t *) param->disc_res.prop[n].val) & ESP_BT_COD_FORMAT_TYPE_BIT_MASK) >> ESP_BT_COD_FORMAT_TYPE_BIT_OFFSET;
              cod = major;
              LogMsg("BLUETOOTH:BTC:bluetooth_scan_callback:ESP_BT_GAP_DISC_RES_EVT: %s  device class: %d.%d  ftype: %d",
                     AddressToString(macaddr, 6, false, ':'), major, minor, ftype);
            }
            break;
          case ESP_BT_GAP_DEV_PROP_RSSI:
            /*
               device rssi
            */
            rssi = *((int8_t *) param->disc_res.prop[n].val);
            LogMsg("BLUETOOTH:BTC:bluetooth_scan_callback:ESP_BT_GAP_DISC_RES_EVT: %s  device rssi: %d",
                   AddressToString(macaddr, 6, false, ':'), rssi);
            break;
          case ESP_BT_GAP_DEV_PROP_EIR:
            /*
               device extended inquiry response
            */
            {
              int offset = 0;

              do {
                uint8_t *data = (uint8_t *) param->disc_res.prop[n].val + offset;
                int len = data[0];
                int eir = data[1];

                if (!len || !eir)
                  break;
                switch (eir) {
                  case ESP_BT_EIR_TYPE_SHORT_LOCAL_NAME:
                  case ESP_BT_EIR_TYPE_CMPL_LOCAL_NAME:
                    {
                      int namelen = min((unsigned int) len - 1, sizeof(name) - 1);

                      strncpy(name, (const char*) &data[2], namelen);
                      name[namelen] = '\0';
                      LogMsg("BLUETOOTH:BTC:bluetooth_scan_callback:ESP_BT_GAP_DISC_RES_EVT: %s  extended inquiriy response: device name: %s",
                             AddressToString(macaddr, 6, false, ':'), name);
                    }
                    break;
                  default:
                    LogMsg("BLUETOOTH:BTC:bluetooth_scan_callback:ESP_BT_GAP_DISC_RES_EVT: %s  extended inquiriy response: 0x%x",
                           AddressToString(macaddr, 6, false, ':'), eir);
                }
                offset += len + 1;
              } while (offset < param->disc_res.prop[n].len);
            }
            break;
        }

        /*
           update the device list
        */
        BlootoothDeviceListAdd(BLUETOOTH_DEVTYPE_BTC, macaddr, name, rssi, cod);
      }
      break;
    case ESP_BT_GAP_DISC_STATE_CHANGED_EVT:
      /*
         the state changed
      */
      LogMsg("BLUETOOTH:BTC:bluetooth_scan_callback:ESP_BT_GAP_DISC_STATE_CHANGED_EVT: discovery %s",
             (param->disc_st_chg.state == ESP_BT_GAP_DISCOVERY_STARTED) ? "started" : "stopped");

      if (param->disc_st_chg.state != ESP_BT_GAP_DISCOVERY_STARTED)
        _bluetooth_mode = BLUETOOTH_MODE_BTC_STOPPED;
      break;
    default:
      ;
  }
}

/*
   initiate a bluetooth classic discovery scan
*/
static bool btc_scan_init(void)
{
  LogMsg("BLUETOOTH: init ...");

  /*
     setup the config
  */
  esp_bt_controller_config_t _bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();

  /*
     init the BT controller
  */
  if (esp_bt_controller_init(&_bt_cfg) != ESP_OK) {
    LogMsg("BLUETOOTH:BTC: esp_bt_controller_init() failed");
    return false;
  }
  if (esp_bt_controller_enable(ESP_BT_MODE_BTDM) != ESP_OK) {
    LogMsg("BLUETOOTH:BTC: esp_bt_controller_enable() failed");
    return false;
  }

  /*
     init the bluedroid part of the controller
  */
  if (esp_bluedroid_init() != ESP_OK) {
    LogMsg("BLUETOOTH:BTC: esp_bluedroid_init() failed");
    return false;
  }
  if (esp_bluedroid_enable() != ESP_OK) {
    LogMsg("BLUETOOTH:BTC: esp_bluedroid_enable() failed");
    return false;
  }

  /*
     setup the classic mode scan
  */
  if (esp_bt_gap_register_callback(btc_scan_callback) != ESP_OK) {
    LogMsg("BLUETOOTH:BTC: esp_bt_gap_register_callback() failed");
    return false;
  }
  if (esp_bt_gap_set_scan_mode(ESP_BT_SCAN_MODE_CONNECTABLE_DISCOVERABLE) != ESP_OK) {
    LogMsg("BLUETOOTH:BTC: esp_bt_gap_set_scan_mode() failed");
    return false;
  }
  if (esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY, ESP_BT_GAP_MAX_INQ_LEN, 0) != ESP_OK) {
    LogMsg("BLUETOOTH: esp_bt_gap_start_discovery() failed");
    return false;
  }
  return true;
}

/*
   stop the bluetooth scan and de-init
*/
static void btc_scan_deinit(void)
{
  esp_bt_gap_cancel_discovery();
  esp_bluedroid_disable();
  esp_bluedroid_deinit();
  esp_bt_controller_disable();
  esp_bt_controller_deinit();
}

static void ble_scan_callback(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
  char name[20] = "";
  byte *macaddr = NULL;
  int rssi = 0, cod = 0;

  DbgMsg("BLUETOOTH:BLE:bluetooth_blescan_callback: event=%d", event);

  switch (event) {
    case ESP_GAP_BLE_SCAN_RESULT_EVT:
      LogMsg("BLUETOOTH:BLE:bluetooth_blescan_callback:ESP_GAP_BLE_SCAN_RESULT_EVT: search_evt=%d", param->scan_rst.search_evt);
      switch (param->scan_rst.search_evt) {
        case ESP_GAP_SEARCH_INQ_CMPL_EVT:
        case ESP_GAP_SEARCH_SEARCH_CANCEL_CMPL_EVT:
          /*
             scan is finished
          */
          _bluetooth_mode = BLUETOOTH_MODE_BLE_STOPPED;
          break;
        case ESP_GAP_SEARCH_INQ_RES_EVT:
          /*
             we have a scan result
          */
          /*
            init the information about the device
          */
          macaddr = (byte *) param->scan_rst.bda;
          rssi = param->scan_rst.rssi;
          cod = param->scan_rst.dev_type;

          LogMsg("BLUETOOTH:BLE:bluetooth_blescan_callback:ESP_GAP_BLE_SCAN_RESULT_EVT: bda=%s", AddressToString(param->scan_rst.bda, 6, false, ':'));
          LogMsg("BLUETOOTH:BLE:bluetooth_blescan_callback:ESP_GAP_BLE_SCAN_RESULT_EVT: dev_type=%d", param->scan_rst.dev_type);
          LogMsg("BLUETOOTH:BLE:bluetooth_blescan_callback:ESP_GAP_BLE_SCAN_RESULT_EVT: ble_evt_type=%d", param->scan_rst.ble_evt_type);
          LogMsg("BLUETOOTH:BLE:bluetooth_blescan_callback:ESP_GAP_BLE_SCAN_RESULT_EVT: rssi=%d", param->scan_rst.rssi);

          //dump("BLUETOOTH:BLE:bluetooth_blescan_callback:ESP_GAP_BLE_SCAN_RESULT_EVT: ble_adv", param->scan_rst.ble_adv, ESP_BLE_ADV_DATA_LEN_MAX + ESP_BLE_SCAN_RSP_DATA_LEN_MAX);

          LogMsg("BLUETOOTH:BLE:bluetooth_blescan_callback:ESP_GAP_BLE_SCAN_RESULT_EVT: adv_data_len=%d", param->scan_rst.adv_data_len);
          dump("BLUETOOTH:BLE:bluetooth_blescan_callback:ESP_GAP_BLE_SCAN_RESULT_EVT: adv data", param->scan_rst.ble_adv, param->scan_rst.adv_data_len);

          {
            /*
               scan the advertised data
            */
            int offset = 0;

            do {
              uint8_t *data = (uint8_t *) &param->scan_rst.ble_adv[offset];
              int len = data[0];
              int rsp = data[1];

              if (!len || !rsp)
                break;
              switch (rsp) {
                default:
                  LogMsg("BLUETOOTH:BLE:bluetooth_scan_callback:ESP_BT_GAP_DISC_RES_EVT: %s  response: 0x%x",
                         AddressToString(macaddr, 6, false, ':'), rsp);
              }
              offset += len + 1;
            } while (offset < param->scan_rst.scan_rsp_len);
          }

          LogMsg("BLUETOOTH:BLE:bluetooth_blescan_callback:ESP_GAP_BLE_SCAN_RESULT_EVT: scan_rsp_len=%d", param->scan_rst.scan_rsp_len);
          dump("BLUETOOTH:BLE:bluetooth_blescan_callback:ESP_GAP_BLE_SCAN_RESULT_EVT: scan response", &param->scan_rst.ble_adv[param->scan_rst.adv_data_len], param->scan_rst.scan_rsp_len);

          {
            /*
               scan the scan response data
            */
            int offset = param->scan_rst.adv_data_len;

            do {
              uint8_t *data = (uint8_t *) &param->scan_rst.ble_adv[offset];
              int len = data[0];
              int rsp = data[1];

              if (!len || !rsp)
                break;
              switch (rsp) {
                case ESP_BLE_AD_TYPE_NAME_SHORT:
                case ESP_BLE_AD_TYPE_NAME_CMPL:
                  {
                    int namelen = min((unsigned int) len - 1, sizeof(name) - 1);

                    strncpy(name, (const char*) &data[2], namelen);
                    name[namelen] = '\0';
                    LogMsg("BLUETOOTH:BLE:bluetooth_scan_callback:ESP_GAP_BLE_SCAN_RESULT_EVT: %s  response: device name: %s",
                           AddressToString(macaddr, 6, false, ':'), name);
                  }
                  break;
                default:
                  LogMsg("BLUETOOTH:BLE:bluetooth_scan_callback:ESP_BT_GAP_DISC_RES_EVT: %s  response: 0x%x",
                         AddressToString(macaddr, 6, false, ':'), rsp);
              }
              offset += len + 1;
            } while (offset < param->scan_rst.scan_rsp_len);
          }

          LogMsg("BLUETOOTH:BLE:bluetooth_blescan_callback:ESP_GAP_BLE_SCAN_RESULT_EVT: flag=0x%x", param->scan_rst.flag);
          LogMsg("BLUETOOTH:BLE:bluetooth_blescan_callback:ESP_GAP_BLE_SCAN_RESULT_EVT: num_resps=%d", param->scan_rst.num_resps);


          LogMsg("BLUETOOTH:BLE:bluetooth_blescan_callback:ESP_GAP_BLE_SCAN_RESULT_EVT: num_dis=%lu", param->scan_rst.num_dis);
#if 0
          esp_bd_addr_t bda;                          /*!< Bluetooth device address which has been searched */
          esp_bt_dev_type_t dev_type;                 /*!< Device type */
          esp_ble_addr_type_t ble_addr_type;          /*!< Ble device address type */
          esp_ble_evt_type_t ble_evt_type;            /*!< Ble scan result event type */
          int rssi;                                   /*!< Searched device's RSSI */
          uint8_t  ble_adv[ESP_BLE_ADV_DATA_LEN_MAX + ESP_BLE_SCAN_RSP_DATA_LEN_MAX];     /*!< Received EIR */
          int flag;                                   /*!< Advertising data flag bit */
          int num_resps;                              /*!< Scan result number */
          uint8_t adv_data_len;                       /*!< Adv data length */
          uint8_t scan_rsp_len;                       /*!< Scan response length */
          uint32_t num_dis;                          /*!< The number of discard packets */
#endif
          break;
        default:
          ;
      }

      /*
         update the device list
      */
      BlootoothDeviceListAdd(BLUETOOTH_DEVTYPE_BLE, macaddr, name, rssi, cod);
      break;
    case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
      /*
         the state changed
      */
      LogMsg("BLUETOOTH:BLE:bluetooth_blescan_callback:ESP_GAP_BLE_SCAN_START_COMPLETE_EVT: scan started");

      _bluetooth_mode = BLUETOOTH_MODE_BLE_STOPPED;
      break;
    default:
      ;
  }
}

/*
   initiate a bluetooth classic discovery scan
*/
static bool ble_scan_init(void)
{
  LogMsg("BLUETOOTH:BLE: init ...");

  /*
     setup the config
  */
  esp_bt_controller_config_t _bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();

  /*
     init the BT controller
  */
  if (esp_bt_controller_init(&_bt_cfg) != ESP_OK) {
    LogMsg("BLUETOOTH:BLE: esp_bt_controller_init() failed");
    return false;
  }
  if (esp_bt_controller_enable(ESP_BT_MODE_BTDM) != ESP_OK) {
    LogMsg("BLUETOOTH:BLE: esp_bt_controller_enable() failed");
    return false;
  }

  /*
     init the bluedroid part of the controller
  */
  if (esp_bluedroid_init() != ESP_OK) {
    LogMsg("BLUETOOTH:BLE: esp_bluedroid_init() failed");
    return false;
  }
  if (esp_bluedroid_enable() != ESP_OK) {
    LogMsg("BLUETOOTH:BLE: esp_bluedroid_enable() failed");
    return false;
  }

  /*
     setup the classic mode scan
  */
  if (esp_ble_gap_register_callback(ble_scan_callback) != ESP_OK) {
    LogMsg("BLUETOOTH:BLE: esp_ble_gap_register_callback() failed");
    return false;
  }
  esp_ble_scan_params_t params;
  params.scan_type = BLE_SCAN_TYPE_ACTIVE;
  params.own_addr_type = BLE_ADDR_TYPE_PUBLIC;
  params.scan_filter_policy = BLE_SCAN_FILTER_ALLOW_ALL;
  params.scan_interval = 5;
  params.scan_window = 100;
  params.scan_duplicate = BLE_SCAN_DUPLICATE_DISABLE;

  if (esp_ble_gap_set_scan_params(&params) != ESP_OK) {
    LogMsg("BLUETOOTH:BLE: esp_ble_gap_set_scan_params() failed");
    return false;
  }
  if (esp_ble_gap_start_scanning(60) != ESP_OK) {
    LogMsg("BLUETOOTH:BLE: esp_ble_gap_start_scanning() failed");
    return false;
  }
  return true;
}

/*
   stop the bluetooth scan and de-init
*/
static void ble_scan_deinit(void)
{
  esp_ble_gap_stop_scanning();
  esp_bluedroid_disable();
  esp_bluedroid_deinit();
  esp_bt_controller_disable();
  esp_bt_controller_deinit();
}

/*
   setup
*/
void BluetoothSetup(void)
{
  /*
     check and correct the config
  */
  _config.bluetooth.btc_scan_time = min(max(_config.bluetooth.btc_scan_time, BLUETOOTH_BTC_SCAN_TIME_MIN), BLUETOOTH_BTC_SCAN_TIME_MAX);
  _config.bluetooth.ble_scan_time = min(max(_config.bluetooth.ble_scan_time, BLUETOOTH_BLE_SCAN_TIME_MIN), BLUETOOTH_BLE_SCAN_TIME_MAX);
  _config.bluetooth.pause_time = min(max(_config.bluetooth.pause_time, BLUETOOTH_PAUSE_TIME_MIN), BLUETOOTH_PAUSE_TIME_MAX);
  _config.bluetooth.absence_cycles = min(max(_config.bluetooth.absence_cycles, BLUETOOTH_ABSENCE_CYCLES_MIN), BLUETOOTH_ABSENCE_CYCLES_MAX);
  _config.bluetooth.publish_absence = _config.bluetooth.publish_absence ? true : false;

  /*
     set the timeout values in the status table
  */
  LogMsg("BLE: setting up timout values in the status table");
  StateModifyTimeout(STATE_SCANNING, (_config.bluetooth.btc_scan_time + _config.bluetooth.ble_scan_time + 5) * 1000);
  StateModifyTimeout(STATE_PAUSING, _config.bluetooth.pause_time * 1000);
}

/*
   update the current bluetooth scan state
*/
void BluetoothUpdate(void)
{
  switch (_bluetooth_mode) {
    case BLUETOOTH_MODE_BTC_STOPPED:
      /*
        deinit the bluetooth scan
      */
      btc_scan_deinit();
      _bluetooth_mode = BLUETOOTH_MODE_IDLE;

      /*
         trigger the BLE scan
      */
      if (ble_scan_init())
        _bluetooth_mode = BLUETOOTH_MODE_BLE_STARTED;
      else
        _bluetooth_mode = BLUETOOTH_MODE_BLE_STOPPED;
      break;
    case BLUETOOTH_MODE_BLE_STOPPED:
      /*
        deinit the bluetooth BLE scan
      */
      ble_scan_deinit();
      _bluetooth_mode = BLUETOOTH_MODE_IDLE;
      break;
    default:
      ;
  }


  /*
     publish the state
  */
  static unsigned long _last_publish_mqtt = 0;

  if (now() - _last_publish_mqtt > 3) {
    blootoothPublishMQTT();
    _last_publish_mqtt = now();
  }
}

/*
   init the scanning
*/
bool BluetoothStartScan(void)
{
  if (_bluetooth_mode == BLUETOOTH_MODE_IDLE) {
    if (_config.bluetooth.btc_scan_time) {
      /*
         start the BTC scan
      */
      if (btc_scan_init()) {
        _bluetooth_mode = BLUETOOTH_MODE_BTC_STARTED;
        return true;
      }
    }
    else
      _bluetooth_mode = BLUETOOTH_MODE_BTC_STOPPED;
  }

  return false;
}/**/
