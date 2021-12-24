#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <configuration.pb.h>
#include <stdint.h>

extern my_Configuration sys_cfg;

#define CFG_OK 		0
#define CFG_ERR 	1

uint8_t cfgRead();
uint8_t cfgWrite();

#endif /* APPLICATION_USER_CORE_FLASH_H_ */
