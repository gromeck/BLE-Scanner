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

#include <stdio.h>
#include <string.h>
#include "config.h"
#include "eeprom.h"

bool _config_mode = false;
static CONFIG _config;

/*
    setup the configuration
*/
void ConfigSetup(void)
{
  /*
   * read the full config from the EEPROM
   */
  memset(&_config,0,sizeof(CONFIG));
  EepromInit(sizeof(CONFIG));
  EepromRead(0,sizeof(CONFIG),&_config);

  if (DBG)
    dump("CFG:",&_config,sizeof(CONFIG));

  /*
   * check if the config version is ok
   */
  if (strcmp(_config.magic,CONFIG_MAGIC) || _config.version != CONFIG_VERSION) {
    /*
     * set a new default configuration
     */
    LogMsg("CFG: unexpected magic and version found -- erasing config and entering config mode");
    memset(&_config,0,sizeof(CONFIG));
    strcpy(_config.magic,CONFIG_MAGIC);
    _config.version = CONFIG_VERSION;  
    _config_mode = true;
    StateChange(STATE_CONFIGURING);
  }
}

/*
   cyclic update of the configuration
*/
void ConfigUpdate(void)
{

}


/*
 * functions to get the configuration for a subsystem
 */
void ConfigGet(int offset,int size,void *cfg)
{
  memcpy(cfg,&_config + offset,size);
}

/*
 * functions to set the configuration for a subsystem -- will be written to the EEPROM
 */
void ConfigSet(int offset,int size,void *cfg)
{
  memcpy(&_config + offset,cfg,size);
  EepromWrite(offset,size,&_config + offset);
}/**/
