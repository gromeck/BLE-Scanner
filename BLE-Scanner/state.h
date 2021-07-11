/*
  BLE-Scanner

  (c) 2020 Christian.Lorenz@gromeck.de

  module to handle the state maschine


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

#ifndef __STATE_H__
#define __STATE_H__ 1

/*
   the device will reboot if there is inactivity for this period in configuration mode
*/
#define STATE_CONFIGURING_TIMEOUT   (5 * 60)

/*
   STATE handling

   if we are in state configuring NTP, MQTT, BLE and BasicAuth are disabled
*/
enum STATE {
  STATE_NONE = 0,
  STATE_SCANNING,
  STATE_PAUSING,
  STATE_CONFIGURING,
  STATE_WAIT_BEFORE_REBOOTING,
  STATE_REBOOT,
};

/*
   setup the state maschine
*/
void StateSetup(int state);

/*
   do the cyclic processing

   return the current state when ever we entered a new state
*/
int StateUpdate(void);

/*
   change the state manually
*/
void StateChange(int state);

/*
   change the timeout of a state
*/
void StateModifyTimeout(int state, unsigned int timeout);

/*
   check if we are in a certain state
*/
bool StateCheck(int state);

#endif
