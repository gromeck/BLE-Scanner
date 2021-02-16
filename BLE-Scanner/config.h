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

#include "git-version.h"

/*
 * enable/disable features
 */


#define __TITLE__   "BLE-Scanner"

/*
   control the debugging messages
*/
#define DBG         1
#define DBG_DUMP    (DBG && 0)

/*
  tags to mark the configuration in the EEPROM
*/
#define CONFIG_MAGIC      __TITLE__ "-CONFIG"
#define CONFIG_VERSION    3

/*
   sub-systems config structs
*/
typedef struct _config_device {
  char name[64];
  char password[64];
  char reserved[256];
} CONFIG_DEVICE;

typedef struct _config_wifi {
  char ssid[64];
  char psk[64];
} CONFIG_WIFI;

typedef struct _config_ntp {
  char server[64];
} CONFIG_NTP;

typedef struct _config_mqtt {
  char server[64];
  int port;
  char user[64];
  char password[64];
  char clientID[64];
  char topicPrefix[64];
  char reserved[192];
} CONFIG_MQTT;

typedef struct _config_bluetooth {
  int btc_scan_time;
  int ble_scan_time;
  int pause_time;
  int absence_cycles;
  bool publish_absence;
  char reserverd[252];
} CONFIG_BT;

/*
   the configuration layout
*/
typedef struct _config {
  char magic[sizeof(CONFIG_MAGIC) + 1];
  int version;
  CONFIG_DEVICE device;
  CONFIG_WIFI wifi;
  CONFIG_NTP ntp;
  CONFIG_MQTT mqtt;
  CONFIG_BT bluetooth;
} CONFIG;

/*
   the config is global
*/
extern CONFIG _config;

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
#define CONFIG_GET(type,name,cfg)  ConfigGet(offsetof(CONFIG,name),sizeof(CONFIG_ ## type),(void *) (cfg))
void ConfigGet(int offset, int size, void *cfg);

/*
   functions to set the configuration for a subsystem -- will be written to the EEPROM
*/
#define CONFIG_SET(type,name,cfg)  ConfigSet(offsetof(CONFIG,name),sizeof(type),(void *) (cfg))
void ConfigSet(int offset, int size, void *cfg);

#endif
