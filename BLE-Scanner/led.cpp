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

#include <Arduino.h>
#include "config.h"
#include "led.h"
#include "util.h"

static int _led_mode = LED_MODE_OFF;
static bool _led_state = false;
static unsigned long _led_last_blink = false;

/*
   setup the led
*/
void LedSetup(int led_mode)
{
  pinMode(LED_PIN, OUTPUT);
  LedMode(led_mode);
}

/*
   do the cyclic update
*/
void LedUpdate(void)
{
  unsigned long blink_rate = 0;
  unsigned long now = millis();

  switch (_led_mode) {
    case LED_MODE_BLINK_SLOW:
      blink_rate = LED_BLINK_RATE_SLOW(_led_state);
      break;
    case LED_MODE_BLINK_FAST:
      blink_rate = LED_BLINK_RATE_FAST(_led_state);
      break;
  }

#if DBG_LED
  DbgMsg("LED: _led_mode=%d  _led_last_blink=%lu  _led_state=%d  now=%lu  blink_rate=%lu", _led_mode, _led_last_blink, _led_state, now, blink_rate);
#endif

  if (blink_rate && now - _led_last_blink > blink_rate) {
    digitalWrite(LED_PIN, _led_state = !_led_state);
    _led_last_blink = now;
  }
}

/*
   set a new led mode
*/
void LedMode(int led_mode)
{
#if DBG_LED
  DbgMsg("LED: _led_mode=%d  _led_last_blink=%lu  _led_state=%d", _led_mode, _led_last_blink, _led_state);
#endif

  switch (_led_mode = led_mode) {
    case LED_MODE_BLINK_SLOW:
    case LED_MODE_BLINK_FAST:
      _led_last_blink = 0;
      break;
  }
  digitalWrite(LED_PIN, _led_state = (_led_mode == LED_MODE_ON) ? true : false);

#if DBG_LED
  DbgMsg("LED: _led_mode=%d  _led_last_blink=%lu  _led_state=%d", _led_mode, _led_last_blink, _led_state);
#endif

  LedUpdate();
}/**/
