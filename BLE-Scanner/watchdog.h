/*
  BLE-Scanner

  (c) 2022 Christian.Lorenz@gromeck.de

  module to handle the watchdog


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

#ifndef __WATCHDOG_H__
#define __WATCHDOG_H__  1

/*
   timeout for the watchdog
*/
#define WATCHDOG_TIMEOUT   10 // [s]


void WatchdogSetup(int timeout);
void WatchdogUpdate(void);

#if UNIT_TEST
void WatchdogUnitTest(void);
#endif

#endif

/**/
