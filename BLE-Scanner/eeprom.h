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

#ifndef __EEPROM_H__
#define __EEPROM_H__ 1

#include <stdio.h>
#include <string.h>
#include <EEPROM.h>
#include "config.h"
#include "util.h"

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
