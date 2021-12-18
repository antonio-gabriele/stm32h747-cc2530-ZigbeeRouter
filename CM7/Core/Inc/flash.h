#ifndef APPLICATION_USER_CORE_FLASH_H_
#define APPLICATION_USER_CORE_FLASH_H_

#include <stdint.h>

struct config_t {
	uint8_t devType;
} __attribute__ ((aligned (4)));

extern struct config_t sys_cfg;

void Flash_Write();
void Flash_Read();

#endif /* APPLICATION_USER_CORE_FLASH_H_ */
