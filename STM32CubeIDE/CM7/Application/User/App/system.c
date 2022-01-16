#include "system.h"

#include <stm32h747i_discovery_qspi.h>
#include <application.h>

#define BUFFER_SIZE         ((uint32_t)0x2000)
#define FLASH_ADDR 			((uint32_t)0x0050)
#define WRITE_ADDR 			((uint32_t)(FLASH_ADDR * 0x2000))
#define READ_ADDR 			(uint8_t*)(0x90000000 + WRITE_ADDR)
#define MAGIC_NUMBER 		((uint8_t)0xAA)

extern Configuration_t sys_cfg;

uint8_t reInit() {
	BSP_QSPI_DeInit(0);
	BSP_QSPI_Init_t init;
	init.InterfaceMode = MT25TL01G_QPI_MODE;
	init.TransferRate = MT25TL01G_DTR_TRANSFER;
	init.DualFlashMode = MT25TL01G_DUALFLASH_ENABLE;
	extern BSP_QSPI_Ctx_t QSPI_Ctx[QSPI_INSTANCES_NUMBER];
	QSPI_Ctx[0].IsInitialized = QSPI_ACCESS_NONE;
	if (BSP_QSPI_Init(0, &init) != BSP_ERROR_NONE) {
		return CFG_ERR;
	}
	return CFG_OK;
}

uint8_t cfgDisableMemoryMappedMode() {
	if (BSP_QSPI_DisableMemoryMappedMode(0) != BSP_ERROR_NONE) {
		return CFG_ERR;
	}
	uint8_t status;
	status = reInit();
	return status;
}

uint8_t cfgEnableMemoryMappedMode() {
	uint8_t status;
	status = reInit();
	if (status != BSP_ERROR_NONE) {
		return CFG_ERR;
	}
	if (BSP_QSPI_EnableMemoryMappedMode(0) != BSP_ERROR_NONE) {
		return CFG_ERR;
	}
	return CFG_OK;
}

uint8_t cfgRead() {
	uint8_t *src = (uint8_t*) (READ_ADDR);
	memcpy(&sys_cfg, src, sizeof(Configuration_t));
	if(sys_cfg.MagicNumber != MAGIC_NUMBER){
		memset(&sys_cfg, 0, sizeof(Configuration_t));
		cfgWrite();
	}
	return CFG_OK;
}

uint8_t cfgWrite() {
	sys_cfg.MagicNumber = MAGIC_NUMBER;
	uint8_t status;
	status = cfgDisableMemoryMappedMode();
	if (status != BSP_ERROR_NONE) {
		return CFG_ERR;
	}
	status = BSP_QSPI_EraseBlock(0, WRITE_ADDR, BSP_QSPI_ERASE_8K);
	if (status != BSP_ERROR_NONE) {
		return CFG_ERR;
	}
	status = BSP_QSPI_Write(0, &sys_cfg, WRITE_ADDR, sizeof(Configuration_t));
	if (status != BSP_ERROR_NONE) {
		return CFG_ERR;
	}
	status = cfgEnableMemoryMappedMode();
	if (status != BSP_ERROR_NONE) {
		return CFG_ERR;
	}
	return CFG_OK;
}
