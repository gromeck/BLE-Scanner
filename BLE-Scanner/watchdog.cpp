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

#include "watchdog.h"
#include "util.h"
#if defined(ESP32)
#include <esp_task_wdt.h>
#endif

static unsigned long _last_update = false;

void WatchdogSetup(int timeout)
{
  timeout += WATCHDOG_TIMEOUT;
  
  LogMsg("WATCHDOG: setting up watchdog with a timeout of %d seconds", timeout);
#if defined(ESP32)
  esp_task_wdt_init(timeout, true);
  // add the current thread
  esp_task_wdt_add(NULL);
#elif defined(ESP8266)
  /*
      NOTE: on the ESP8266 the SW watchdog is weird

      In general the timeout of the HW watchdog is fix at ~8 seconds.
      The timeout of the SW watchdog is normally half the timeout of the HW watchdog.
      It is forseen to configure the SW watchog timeout, but this doesn't work.
            
      As a solution the SW watchdog is disabled, and let the HW watchdog do its job.      
  */
  ESP.wdtDisable();
  //ESP.wdtEnable(0);
  //ESP.wdtEnable(timeout * 1000);
#endif
}

void WatchdogUpdate(void)
{
  unsigned long now = millis();

  if (now - _last_update > (WATCHDOG_TIMEOUT / 2) * 1000) {
    /*
        feed the watchdog
    */
#if DBG_WATCHDOG || UNIT_TEST
    DbgMsg("WATCHDOG: feeding the watchdog");
#endif

#if defined(ESP32)
    esp_task_wdt_reset();
#elif defined(ESP8266)
    ESP.wdtFeed();
#endif

    _last_update = now;
  }
}

#if UNIT_TEST

#define TEST_CYCLES   3

void WatchdogUnitTest(void)
{
  unsigned long _feed_time;

  WatchdogSetup();

  DbgMsg("WATCHDOG: feeding the watchdog for %d cycles ...", TEST_CYCLES);

  for (int n = 0;; n++) {
    if (n < TEST_CYCLES * WATCHDOG_TIMEOUT) {
      WatchdogUpdate();
      _feed_time = millis();
    }
    else {
      DbgMsg("WATCHDOG: no longer feeding the watchdog -- expecting the watchdog to reset the CPU");
    }

    if (millis() - _feed_time > 2 * WATCHDOG_TIMEOUT * 1000) {
      DbgMsg("WATCHDOG: watchdog timeout is two times overdue -- TEST FAILED");
      while (1)
        delay(1000);
    }

    delay(1000);
  }
}

#endif

/**/
