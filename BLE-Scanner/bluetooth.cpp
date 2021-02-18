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
#include "scandev.h"
#include "util.h"

#include "BLEUUID.h"
#include <esp_bt.h>
#include <esp_bt_main.h>
#include <esp_gap_bt_api.h>
#include <esp_gap_ble_api.h>
#include <esp_bt_device.h>

static BLUETOOTH_MODE_T _bluetooth_mode = BLUETOOTH_MODE_IDLE;
static time_t _last_activescan = 0;

static void btc_scan_callback(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
  char name[20] = "";
  byte *macaddr = NULL;
  int rssi = 0, cod = 0;

#if DBG_BT
  DbgMsg("BT:BTC:bluetooth_scan_callback: event=%d", event);
#endif
  switch (event) {
    case ESP_BT_GAP_DISC_RES_EVT:
#if DBG_BT
      DbgMsg("BT:BTC:bluetooth_scan_callback:ESP_BT_GAP_DISC_RES_EVT: %s: properties: %d",
             AddressToString(param->disc_res.bda, 6, false, ':'), param->disc_res.num_prop);
#endif

      /*
         init the information about the device
      */
      macaddr = (byte *) param->disc_res.bda;

      /*
         see what the properties contain
      */
      for (int n = 0; n < param->disc_res.num_prop; n++) {
#if DBG_BT
        DbgMsg("BT:BTC:bluetooth_scan_callback:ESP_BT_GAP_DISC_RES_EVT: %s: prop[%d]: type=%d",
               AddressToString(macaddr, 6, false, ':'), n, param->disc_res.prop[n].type);
        dump("BT:BTC:bluetooth_scan_callback:ESP_BT_GAP_DISC_RES_EVT: prop", param->disc_res.prop[n].val, param->disc_res.prop[n].len);
#endif
        switch (param->disc_res.prop[n].type) {
          case ESP_BT_GAP_DEV_PROP_BDNAME:
            /*
               device name
            */
#if DBG_BT
            DbgMsg("BT:BTC:bluetooth_scan_callback:ESP_BT_GAP_DISC_RES_EVT: %s  device name: %s",
                   AddressToString(macaddr, 6, false, ':'), (char *) param->disc_res.prop[n].val);
#endif
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
#if DBG_BT
              DbgMsg("BT:BTC:bluetooth_scan_callback:ESP_BT_GAP_DISC_RES_EVT: %s  device class: %d.%d  ftype: %d",
                     AddressToString(macaddr, 6, false, ':'), major, minor, ftype);
#endif
            }
            break;
          case ESP_BT_GAP_DEV_PROP_RSSI:
            /*
               device rssi
            */
            rssi = *((int8_t *) param->disc_res.prop[n].val);
#if DBG_BT
            DbgMsg("BT:BTC:bluetooth_scan_callback:ESP_BT_GAP_DISC_RES_EVT: %s  device rssi: %d",
                   AddressToString(macaddr, 6, false, ':'), rssi);
#endif
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
#if DBG_BT
                      DbgMsg("BT:BTC:bluetooth_scan_callback:ESP_BT_GAP_DISC_RES_EVT: %s  extended inquiriy response: device name: %s",
                             AddressToString(macaddr, 6, false, ':'), name);
#endif
                    }
                    break;
                  default:
#if DBG_BT
                    DbgMsg("BT:BTC:bluetooth_scan_callback:ESP_BT_GAP_DISC_RES_EVT: %s  extended inquiriy response: 0x%x",
                           AddressToString(macaddr, 6, false, ':'), eir);
#endif
                    ;
                }
                offset += len + 1;
              } while (offset < param->disc_res.prop[n].len);
            }
            break;
        }

        /*
           update the device list
        */
        ScanDevAdd(SCANDEV_TYPE_BTC, macaddr, name, rssi, cod);
      }
      break;
    case ESP_BT_GAP_DISC_STATE_CHANGED_EVT:
      /*
         the state changed
      */
#if DBG_BT
      DbgMsg("BT:BTC:bluetooth_scan_callback:ESP_BT_GAP_DISC_STATE_CHANGED_EVT: discovery %s",
             (param->disc_st_chg.state == ESP_BT_GAP_DISCOVERY_STARTED) ? "started" : "stopped");
#endif

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
#if DBG_BT
  DbgMsg("BT:BTC: init ...");
#endif

  /*
     setup the config
  */
  esp_bt_controller_config_t _bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();

  /*
     init the BT controller
  */
  if (esp_bt_controller_init(&_bt_cfg) != ESP_OK) {
    LogMsg("BT:BTC: esp_bt_controller_init() failed");
    return false;
  }
  if (esp_bt_controller_enable(ESP_BT_MODE_BTDM) != ESP_OK) {
    LogMsg("BT:BTC: esp_bt_controller_enable() failed");
    return false;
  }

  /*
     init the bluedroid part of the controller
  */
  if (esp_bluedroid_init() != ESP_OK) {
    LogMsg("BT:BTC: esp_bluedroid_init() failed");
    return false;
  }
  if (esp_bluedroid_enable() != ESP_OK) {
    LogMsg("BT:BTC: esp_bluedroid_enable() failed");
    return false;
  }

  /*
     setup the classic mode scan
  */
  if (esp_bt_gap_register_callback(btc_scan_callback) != ESP_OK) {
    LogMsg("BT:BTC: esp_bt_gap_register_callback() failed");
    return false;
  }
  if (esp_bt_gap_set_scan_mode(ESP_BT_SCAN_MODE_CONNECTABLE_DISCOVERABLE) != ESP_OK) {
    LogMsg("BT:BTC: esp_bt_gap_set_scan_mode() failed");
    return false;
  }
  if (esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY, ESP_BT_GAP_MAX_INQ_LEN, 0) != ESP_OK) {
    LogMsg("BT: esp_bt_gap_start_discovery() failed");
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
  char name[SCANDEV_NAME_LENGTH] = "";
  byte *macaddr = NULL;
  int rssi = 0, cod = 0;

  switch (event) {
    case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT:
      /*
         parameters are complete
      */
#if DBG_BT
      DbgMsg("BT:BLE:ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT: paramters complete");
#endif
      break;
    case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
      /*
         scan started
      */
#if DBG_BT
      DbgMsg("BT:BLE:ESP_GAP_BLE_SCAN_START_COMPLETE_EVT: scan started");
#endif
      break;
    case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
      /*
         scan stopped
      */
#if DBG_BT
      DbgMsg("BT:BLE:ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT: scan stopped");
#endif
      _bluetooth_mode = BLUETOOTH_MODE_BLE_STOPPED;
      break;
    case ESP_GAP_BLE_SCAN_RESULT_EVT:
      /*
         received a scan event
      */
      switch (param->scan_rst.search_evt) {
        case ESP_GAP_SEARCH_INQ_CMPL_EVT:
#if DBG_BT
          DbgMsg("BT:BLE:ESP_GAP_BLE_SCAN_RESULT_EVT:ESP_GAP_SEARCH_INQ_CMPL_EVT: stopping scanning");
#endif
          _bluetooth_mode = BLUETOOTH_MODE_BLE_STOPPED;
          break;
        case ESP_GAP_SEARCH_SEARCH_CANCEL_CMPL_EVT:
          /*
             scan is finished
          */
#if DBG_BT
          DbgMsg("BT:BLE:ESP_GAP_BLE_SCAN_RESULT_EVT:ESP_GAP_SEARCH_SEARCH_CANCEL_CMPL_EVT: stopping scanning");
#endif
          _bluetooth_mode = BLUETOOTH_MODE_BLE_STOPPED;
          break;
        case ESP_GAP_SEARCH_INQ_RES_EVT:
          /*
             we have a scan result
          */
#if DBG_BT_DETAIL
          DbgMsg("BT:BLE:ESP_GAP_BLE_SCAN_RESULT_EVT:ESP_GAP_SEARCH_INQ_RES_EVT: received a scan result");
#endif
          /*
            init the information about the device
          */
          macaddr = (byte *) param->scan_rst.bda;
          rssi = param->scan_rst.rssi;
          cod = param->scan_rst.dev_type;
#if DBG_BT_DETAIL
          DbgMsg("BT:BLE:ESP_GAP_BLE_SCAN_RESULT_EVT:ESP_GAP_SEARCH_INQ_RES_EVT: bda=%s", AddressToString(param->scan_rst.bda, 6, false, ':'));
          DbgMsg("BT:BLE:ESP_GAP_BLE_SCAN_RESULT_EVT:ESP_GAP_SEARCH_INQ_RES_EVT: dev_type=%d", param->scan_rst.dev_type);
          DbgMsg("BT:BLE:ESP_GAP_BLE_SCAN_RESULT_EVT:ESP_GAP_SEARCH_INQ_RES_EVT: ble_evt_type=%d", param->scan_rst.ble_evt_type);
          DbgMsg("BT:BLE:ESP_GAP_BLE_SCAN_RESULT_EVT:ESP_GAP_SEARCH_INQ_RES_EVT: rssi=%d", param->scan_rst.rssi);
#endif
          {
            /*
               scan the advertised data
            */
#if DBG_BT_DETAIL
            DbgMsg("BT:BLE:ESP_GAP_BLE_SCAN_RESULT_EVT:ESP_GAP_SEARCH_INQ_RES_EVT: adv_data_len=%d", param->scan_rst.adv_data_len);
            dump("BT:BLE:ESP_GAP_BLE_SCAN_RESULT_EVT:ESP_GAP_SEARCH_INQ_RES_EVT: ble_adv", param->scan_rst.ble_adv, param->scan_rst.adv_data_len);
#endif
            int offset = 0;

            do {
              uint8_t *data = (uint8_t *) &param->scan_rst.ble_adv[offset];
              int len = data[0];
              int rsp = data[1];

#if DBG_BT_DETAIL
              LogMsg("BT:BLE:ESP_GAP_BLE_SCAN_RESULT_EVT:ESP_GAP_SEARCH_INQ_RES_EVT: %s  len=%d  res=0x%x",
                     AddressToString(macaddr, 6, false, ':'), len, rsp);
#endif
              if (!len || !rsp)
                break;

              /*
                 handle the resonses
              */

              offset += len + 1;
            } while (offset < param->scan_rst.adv_data_len);
          }
          {
            /*
               scan the scan response data
            */
#if DBG_BT_DETAIL
            DbgMsg("BT:BLE:ESP_GAP_BLE_SCAN_RESULT_EVT:ESP_GAP_SEARCH_INQ_RES_EVT: scan_rsp_len=%d", param->scan_rst.scan_rsp_len);
            dump("BT:BLE:ESP_GAP_BLE_SCAN_RESULT_EVT:ESP_GAP_SEARCH_INQ_RES_EVT: scan response", &param->scan_rst.ble_adv[param->scan_rst.adv_data_len], param->scan_rst.scan_rsp_len);
#endif
            int offset = 0;

            do {
              uint8_t *data = (uint8_t *) &param->scan_rst.ble_adv[param->scan_rst.adv_data_len + offset];
              int len = data[0];
              int rsp = data[1];

#if DBG_BT_DETAIL
              LogMsg("BT:BLE:ESP_GAP_BLE_SCAN_RESULT_EVT:ESP_GAP_SEARCH_INQ_RES_EVT: %s  len=%d  res=0x%x",
                     AddressToString(macaddr, 6, false, ':'), len, rsp);
#endif
              if (!len || !rsp)
                break;

              /*
                 handle the responses
              */
              switch (rsp) {
                case ESP_BLE_AD_TYPE_NAME_SHORT:
                case ESP_BLE_AD_TYPE_NAME_CMPL:
                  {
                    int namelen = min((unsigned int) len - 1, sizeof(name) - 1);

                    strncpy(name, (const char*) &data[2], namelen);
                    name[namelen] = '\0';
#if DBG_BT_DETAIL
                    DbgMsg("BT:BLE:ESP_GAP_BLE_SCAN_RESULT_EVT:ESP_GAP_SEARCH_INQ_RES_EVT: %s  response: device name: %s",
                           AddressToString(macaddr, 6, false, ':'), name);
#endif
                  }
                  break;
              }

              offset += len + 1;
            } while (offset < param->scan_rst.scan_rsp_len);
          }

#if DBG_BT_DETAIL
          LogMsg("BT:BLE:ESP_GAP_BLE_SCAN_RESULT_EVT:ESP_GAP_SEARCH_INQ_RES_EVT: flag=0x%x", param->scan_rst.flag);
          LogMsg("BT:BLE:ESP_GAP_BLE_SCAN_RESULT_EVT:ESP_GAP_SEARCH_INQ_RES_EVT: num_resps=%d", param->scan_rst.num_resps);
          LogMsg("BT:BLE:ESP_GAP_BLE_SCAN_RESULT_EVT:ESP_GAP_SEARCH_INQ_RES_EVT: num_dis=%lu", param->scan_rst.num_dis);
#endif
          /*
             update the device list
          */
#if DBG_BT
          LogMsg("BT:BLE:ESP_GAP_BLE_SCAN_RESULT_EVT:ESP_GAP_SEARCH_INQ_RES_EVT: macaddr=%s  name=%-20s  rssi=%3d  cod=%3d",
                 AddressToString(param->scan_rst.bda, 6, false, ':'),
                 name, rssi, cod);
#endif
          ScanDevAdd(SCANDEV_TYPE_BLE, macaddr, name, rssi, cod);
          break;
        default:
#if DBG_BT
          DbgMsg("BT:BLE:ESP_GAP_BLE_SCAN_RESULT_EVT: unhandled event=%d", param->scan_rst.search_evt);
#endif
          ;
      }
      break;
    default:
#if DBG_BT
      DbgMsg("BT:BLE: unhandled event=%d", event);
#endif
      ;
  }
}

/*
   initiate a bluetooth classic discovery scan
*/
static bool ble_scan_init(void)
{
  LogMsg("BT:BLE: init ...");
  int rc;

  /*
     setup the config
  */
  esp_bt_controller_config_t _bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();

  /*
     init the BT controller
  */
  if ((rc = esp_bt_controller_init(&_bt_cfg)) != ESP_OK) {
    LogMsg("BT:BLE: esp_bt_controller_init() failed: %d", rc);
    return false;
  }
  if ((rc = esp_bt_controller_enable(ESP_BT_MODE_BTDM /* ESP_BT_MODE_BLE */)) != ESP_OK) {
    LogMsg("BT:BLE: esp_bt_controller_enable() failed: %d", rc);
    return false;
  }

  /*
     init the bluedroid part of the controller
  */
  if ((rc = esp_bluedroid_init()) != ESP_OK) {
    LogMsg("BT:BLE: esp_bluedroid_init() failed: %d", rc);
    return false;
  }
  if ((rc = esp_bluedroid_enable()) != ESP_OK) {
    LogMsg("BT:BLE: esp_bluedroid_enable() failed: %d", rc);
    return false;
  }

  /*
     setup the LE scan
  */
  if ((rc = esp_ble_gap_register_callback(ble_scan_callback)) != ESP_OK) {
    LogMsg("BT:BLE: esp_ble_gap_register_callback() failed: %d", rc);
    return false;
  }
  esp_ble_scan_params_t params;
  params.own_addr_type = BLE_ADDR_TYPE_PUBLIC;
  params.scan_filter_policy = BLE_SCAN_FILTER_ALLOW_ALL;

  /*
     active/passive scan
  */
  if (now() - _last_activescan > _config.bluetooth.activescan_timeout) {
    params.scan_type = BLE_SCAN_TYPE_ACTIVE;
    _last_activescan = now();
  }
  else
    params.scan_type = BLE_SCAN_TYPE_PASSIVE;

  /*
     timing of the scan
  */
  params.scan_interval = (int) (3.0 /*s*/ * 1000.0 /*ms*/ / 0.625 /*ms*/);
  //params.scan_interval = 10000;
  params.scan_window = params.scan_interval;

  /*
    BLE_SCAN_DUPLICATE_DISABLE => report each packet received
    BLE_SCAN_DUPLICATE_ENABLE  => filter out duplicate reports
  */
  //params.scan_duplicate = BLE_SCAN_DUPLICATE_DISABLE;
  params.scan_duplicate = BLE_SCAN_DUPLICATE_ENABLE;

  if ((rc = esp_ble_gap_set_scan_params(&params)) != ESP_OK) {
    LogMsg("BT:BLE: esp_ble_gap_set_scan_params() failed: %d", rc);
    return false;
  }
  if ((rc = esp_ble_gap_start_scanning(_config.bluetooth.ble_scan_time)) != ESP_OK) {
    LogMsg("BT:BLE: esp_ble_gap_start_scanning() failed: %d", rc);
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
  FIX_RANGE(_config.bluetooth.btc_scan_time,BLUETOOTH_BTC_SCAN_TIME_MIN,BLUETOOTH_BTC_SCAN_TIME_MAX);
  FIX_RANGE(_config.bluetooth.ble_scan_time,BLUETOOTH_BLE_SCAN_TIME_MIN,BLUETOOTH_BLE_SCAN_TIME_MAX);
  FIX_RANGE(_config.bluetooth.pause_time,BLUETOOTH_PAUSE_TIME_MIN,BLUETOOTH_PAUSE_TIME_MAX);
  FIX_RANGE(_config.bluetooth.activescan_timeout,BLUETOOTH_ACTIVESCAN_TIMEOUT_MIN,BLUETOOTH_ACTIVESCAN_TIMEOUT_MAX);
  FIX_RANGE(_config.bluetooth.absence_cycles,BLUETOOTH_ABSENCE_CYCLES_MIN,BLUETOOTH_ABSENCE_CYCLES_MAX);
  _config.bluetooth.publish_absence = _config.bluetooth.publish_absence ? true : false;
  FIX_RANGE(_config.bluetooth.publish_timeout,BLUETOOTH_PUBLISH_TIMEOUT_MIN,BLUETOOTH_PUBLISH_TIMEOUT_MAX);

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
