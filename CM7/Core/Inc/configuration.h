#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <configuration.pb.h>

#include <stdint.h>

extern my_Configuration sys_cfg;

void cfgRead();
void cfgWrite();

#endif /* APPLICATION_USER_CORE_FLASH_H_ */
