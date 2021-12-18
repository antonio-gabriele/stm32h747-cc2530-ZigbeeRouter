#include "flash.h"

#include <stm32h7xx_hal.h>

#define STARTSECTORADDRESS 0x080E0000

struct config_t sys_cfg;

void Flash_Write() {

}

void Flash_Read() {
	int numberofwords = (sizeof(struct config_t) >> 2);
	numberofwords += (sizeof(struct config_t) & 0x03) != 0;
	int pageAddress = STARTSECTORADDRESS;
	uint32_t *buffer = (uint32_t*) (&sys_cfg);

	while (numberofwords-- > 0) {
		*buffer = *(uint32_t*) pageAddress;
		pageAddress += 4;
		buffer++;
	}
}
