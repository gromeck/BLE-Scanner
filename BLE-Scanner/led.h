/*
  BLE-Scanner

  (c) 2020 Christian.Lorenz@gromeck.de

  module to handle the LED


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

#ifndef __LED_H__
#define __LED_H__ 1

#include "config.h"

/*
   which oin to use?
*/
#define LED_PIN   2

/*
   define some LED states
*/
enum LED_MODE {
  LED_MODE_OFF,
  LED_MODE_ON,
  LED_MODE_BLINK_SLOW,
  LED_MODE_BLINK_FAST,
};

/*
   define the blink rates in milli seconds
*/
#define LED_BLINK_RATE_SLOW(state)    ((state) ? 1000 : 1000)
#define LED_BLINK_RATE_FAST(state)    ((state) ?  100  : 100)

/*
   setup the led
*/
void LedSetup(int led_mode);

/*
   do the cyclic update
*/
void LedUpdate(void);

/*
   set a new led mode
*/
void LedMode(int led_mode);

#endif

/**/
