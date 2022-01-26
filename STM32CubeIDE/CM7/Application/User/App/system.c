#include "system.h"

#include <stm32h747i_discovery_qspi.h>
#include <application.h>
#include <fatfs.h>
#include <stdio.h>
#include <string.h>

#define BUFFER_SIZE         ((uint32_t)0x2000)
#define FLASH_ADDR 			((uint32_t)0x0050)
#define WRITE_ADDR 			((uint32_t)(FLASH_ADDR * 0x2000))
#define READ_ADDR 			(uint8_t*)(0x90000000 + WRITE_ADDR)
#define MAGIC_NUMBER 		((uint8_t)0xAA)

extern Configuration_t sys_cfg;

uint8_t cfgRead(void *  none) {
	return CFG_OK;
}

uint8_t cfgWrite(void * none) {
	FATFS fs;
	FRESULT res;
	UINT bytesWritten;
	char writeBuff[1024];
	res = f_mount(&fs, "0:", 1);
	if (res != FR_OK) {
		return CFG_ERR;
	}
	FIL logFile;
	res = f_open(&logFile, "config.txt", FA_CREATE_ALWAYS | FA_WRITE);
	if (res != FR_OK) {
		return CFG_ERR;
	}
	snprintf(writeBuff, sizeof(writeBuff), "{ \"nodes\" : [\n");
	bytesWritten = strlen(writeBuff);
	res = f_write(&logFile, writeBuff, bytesWritten, &bytesWritten);
	if (res != FR_OK) {
		return CFG_ERR;
	}
	uint8_t iNode;
	for (iNode = 0; iNode < sys_cfg.NodesCount; iNode++) {
		Node_t *node = &sys_cfg.Nodes[iNode];
		snprintf(writeBuff, sizeof(writeBuff), "\t{ \"Address\" : %u, \"IEEE\" : %llu, \"ManufacturerName\" : \"%s\" }, \n", node->Address, node->IEEE, node->ManufacturerName);
		bytesWritten = strlen(writeBuff);
		res = f_write(&logFile, writeBuff, bytesWritten, &bytesWritten);
		if (res != FR_OK) {
			return CFG_ERR;
		}

	}
	snprintf(writeBuff, sizeof(writeBuff), "]\n}");
	bytesWritten = strlen(writeBuff);
	res = f_write(&logFile, writeBuff, bytesWritten, &bytesWritten);
	if (res != FR_OK) {
		return CFG_ERR;
	}
	res = f_close(&logFile);
	if (res != FR_OK) {
		return CFG_ERR;
	}
	res = f_mount(NULL, "", 0);
	if (res != FR_OK) {
		return CFG_ERR;
	}
	return CFG_OK;
}
