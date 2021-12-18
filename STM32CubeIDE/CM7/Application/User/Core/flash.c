#include "flash.h"

#include <stm32h7xx_hal.h>

#define STARTSECTORADDRESS 0x080E0000

struct config_t sys_cfg;

void Flash_Write() {
	int numberofwords = (sizeof(struct config_t) >> 2);
	numberofwords += (sizeof(struct config_t) & 0x03) != 0;
	int pageAddress = STARTSECTORADDRESS;
	uint32_t *buffer = (uint32_t*) (&sys_cfg);

	static FLASH_EraseInitTypeDef EraseInitStruct;
	uint32_t PAGEError;

	HAL_FLASH_Unlock();

	uint32_t StartPage = GetPage(pageAddress);
	uint32_t EndPageAdress = pageAddress + numberofwords * 4;
	uint32_t EndPage = GetPage(EndPageAdress);

	/* Fill EraseInit structure*/
	EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
	EraseInitStruct.PageAddress = StartPage;
	EraseInitStruct.NbPages = ((EndPage - StartPage) / FLASH_BANK_SIZE) + 1;
	FLASH_SECTOR_SIZE

	if (HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError) != HAL_OK) {
		/*Error occurred while page erase.*/
		return HAL_FLASH_GetError();
	}

	while (numberofwords-- > 0) {
		if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, pageAddress, *buffer) == HAL_OK) {
			pageAddress += 4; // use StartPageAddress += 2 for half word and 8 for double word
			buffer++;
		} else {
			return HAL_FLASH_GetError();
		}
	}

	/* Lock the Flash to disable the flash control register access (recommended
	 to protect the FLASH memory against possible unwanted operation) *********/
	HAL_FLASH_Lock();
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
