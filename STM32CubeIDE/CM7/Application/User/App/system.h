#ifndef APPLICATION_USER_APP_CONFIGURATION_H_
#define APPLICATION_USER_APP_CONFIGURATION_H_

#define CFG_ERR 1
#define CFG_OK 0

#include <stdint.h>

uint8_t cfgRead();
uint8_t cfgWrite();

#endif /* APPLICATION_USER_APP_CONFIGURATION_H_ */
