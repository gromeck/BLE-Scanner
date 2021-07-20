/*
  BLE-Scanner

  (c) 2020 Christian.Lorenz@gromeck.de

  module to handle the configuration


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

#ifndef __CONFIG_H__
#define __CONFIG_H__ 1

#if __has_include("git-version.h")
#include "git-version.h"
#else
#define GIT_VERSION		"unknown"
#endif

#define __TITLE__   "BLE-Scanner"

/*
   control the debugging messages
*/
#define DBG               1
#define DBG_BT            (DBG && 1)
#define DBG_CFG           (DBG && 0)
#define DBG_HTTP          (DBG && 0)
#define DBG_LED           (DBG && 0)
#define DBG_MANUFACTURER  (DBG && 0)
#define DBG_NTP           (DBG && 0)
#define DBG_MQTT          (DBG && 1)
#define DBG_SCANDEV       (DBG && 0)
#define DBG_STATE         (DBG && 0)


/*
  tags to mark the configuration in the EEPROM
*/
#define CONFIG_MAGIC      __TITLE__ "-CONFIG"
#define CONFIG_VERSION    5

/*
   sub-systems config structs
*/
typedef struct _config_wifi {
  char ssid[64];
  char psk[64];
} CONFIG_WIFI_T;

typedef struct _config_device {
  char name[64];
  char password[64];
  char reserved[64];
} CONFIG_DEVICE_T;

typedef struct _config_ntp {
  char server[64];
  int timezone;
} CONFIG_NTP_T;

typedef struct _config_mqtt {
  char server[64];
  int port;
  char user[64];
  char password[64];
  char clientID[64];
  char topicPrefix[64];
  bool publish_absence;             // only report presence, or also the absence
  unsigned long publish_timeout;    // don't report a device too often
  char reserved[58];
} CONFIG_MQTT_T;

typedef struct _config_bluetooth {
  unsigned long scan_time;          // duration of the BLE scan in seconds
  unsigned long pause_time;         // pause time after scans before restarting the scans
  unsigned long activescan_timeout; // don't report a device too often
  int absence_cycles;               // number of complete cycles before a device is set absent
  unsigned long battcheck_timeout;  // don't check the device battery too often
  char reserved[70];
} CONFIG_BT_T;

/*
   the configuration layout
*/
typedef struct _config {
  char magic[sizeof(CONFIG_MAGIC) + 1];
  int version;
  CONFIG_WIFI_T wifi;
  CONFIG_DEVICE_T device;
  CONFIG_NTP_T ntp;
  CONFIG_MQTT_T mqtt;
  CONFIG_BT_T bluetooth;
} CONFIG_T;

/*
   the config is global
*/
extern CONFIG_T _config;

/*
    setup the configuration
*/
bool ConfigSetup(void);

/*
   cyclic update of the configuration
*/
void ConfigUpdate(void);

/*
   functions to get the configuration for a subsystem
*/
#define CONFIG_GET(type,name,cfg)  ConfigGet(offsetof(CONFIG_T,name),sizeof(CONFIG_ ## type ## _T),(void *) (cfg))
void ConfigGet(int offset, int size, void *cfg);

/*
   functions to set the configuration for a subsystem -- will be written to the EEPROM
*/
#define CONFIG_SET(type,name,cfg)  ConfigSet(offsetof(CONFIG_T,name),sizeof(type),(void *) (cfg))
void ConfigSet(int offset, int size, void *cfg);

#endif
