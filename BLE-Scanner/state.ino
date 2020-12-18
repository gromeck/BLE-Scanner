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

#include "config.h"
#include "state.h"
#include "util.h"

static int _state = STATE_NONE;
static int _state_new = STATE_NONE;
static unsigned long _state_timer = 0;

/*
   define the state maschine
*/
typedef struct {
  int state;  // the current state
  int next;   // the next state
  int wait;   // the time in millis seconds when to switch to the next state
} STATES;

static STATES _states[] = {
  /*
     pause time
  */
  { STATE_PAUSING, STATE_SCANNING, BLE_PAUSE_TIME * 1000 },
  
  /*
     normally the state will change from scanning to pausing upon finished scan,
     this entry is just to be sure
  */
  { STATE_SCANNING, STATE_PAUSING, 2 * BLE_SCAN_TIME * 1000 },

  /*
   * in this state we will stay until reboot
   */
  { STATE_CONFIGURING, STATE_CONFIGURING, 10 * 60 * 1000 },
};


/*
   setup the state maschine
*/
void StateSetup(int state)
{
  StateChange(state);
}

/*
   do the cyclic processing

   return the current state when ever we entered a new state
*/
int StateUpdate(void)
{
  unsigned long now = millis();
  int new_state = STATE_NONE;

  if (_state_new != STATE_NONE) {
    /*
       a manual state change was done
    */
    new_state = _state_new;
    _state_new = STATE_NONE;
  }
  else {
    /*
       normal state maschine
    */
    for (int n = 0; n < sizeof(_states) / sizeof(_states[0]); n++) {
      if (_states[n].state == _state) {
        /*
           we found the state in our table
        */
        if ((_state_timer && now > _state_timer) || _states[n].wait <= 0) {
          /*
            timer expired or, there was no wait time, change the state
          */
          new_state = _states[n].next;
        }
        else if (!_state_timer) {
          /*
             start the timer for this state change
          */
          DbgMsg("STATE: starting timer to change from %d to %d in %lums", _state, _states[n].next, _states[n].wait);
          _state_timer = now + _states[n].wait;
        }
        break;
      }
    }
  }

  /*
     did the state change?
  */
  if (new_state != STATE_NONE && new_state != _state) {
    DbgMsg("STATE: changing from %d to %d", _state, new_state);
    _state_timer = 0;
    return _state = new_state;
  }
  return STATE_NONE;
}

/*
   change the state manually
*/
void StateChange(int state)
{
  _state_new = (_state != state) ? state : STATE_NONE;
}/**/
