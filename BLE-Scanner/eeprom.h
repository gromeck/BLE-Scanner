/*
  BLE-Scanner

  (c) 2020 Christian.Lorenz@gromeck.de

  module to handle the EEPROM


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

#ifndef __EPROM_H__
#define __EPROM_H__ 1

#include <stdio.h>
#include <string.h>
#include <EEPROM.h>
#include "config.h"
#include "util.h"

/*
    configuration layout of the EEPROM

    TO BE DELETED
*/
#define EEPROM_ADDR_WIFI_SSID     0
#define EEPROM_SIZE_WIFI_SSID     64
#define EEPROM_ADDR_WIFI_PSK      (EEPROM_ADDR_WIFI_SSID + EEPROM_SIZE_WIFI_SSID)
#define EEPROM_SIZE_WIFI_PSK      64
#define EEPROM_ADDR_NTP_SERVER    (EEPROM_ADDR_WIFI_PSK + EEPROM_SIZE_WIFI_PSK)
#define EEPROM_SIZE_NTP_SERVER    64
#define EEPROM_ADDR_COUNTER_VAL   (EEPROM_ADDR_NTP_SERVER + EEPROM_SIZE_NTP_SERVER)
#define EEPROM_SIZE_COUNTER_VAL   sizeof(double)
#define EEPROM_ADDR_COUNTER_INC   (EEPROM_ADDR_COUNTER_VAL + EEPROM_SIZE_COUNTER_VAL)
#define EEPROM_SIZE_COUNTER_INC   sizeof(double)
#define EEPROM_SIZE               (EEPROM_ADDR_COUNTER_INC + EEPROM_SIZE_COUNTER_INC)

/*
**  init the EEPROM handling
*/
void EepromInit(const int size);

/*
**  dump the EEPROM
*/
void EepromDump(void);

/*
**  clear the EEPROM
*/
void EepromClear(void);

/*
**	read from the EEPROM
**
**	if all bytes were FF, zero is returned
*/
int EepromRead(int addr, int len, void *buffer);

/*
**	write to the EEPROM
*/
void EepromWrite(int addr, int len, const void *buffer);

#endif

/**/
