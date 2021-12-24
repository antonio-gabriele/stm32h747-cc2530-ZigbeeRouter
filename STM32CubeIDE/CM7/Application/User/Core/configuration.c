#include "configuration.h"

#include <pb.h>
#include <pb_decode.h>
#include <pb_encode.h>
#include <stm32h747i_discovery_qspi.h>

my_Configuration sys_cfg = my_Configuration_init_zero;

#define BUFFER_SIZE         ((uint32_t)0x2000)
#define FLASH_ADDR 			((uint32_t)0x0050)
#define WRITE_ADDR 			((uint32_t)(FLASH_ADDR * 0x2000))
#define READ_ADDR 			(uint8_t*)(0x90000000 + WRITE_ADDR)
#define MAGIC_NUMBER 		((uint8_t)0xAA)

uint8_t qspi_aTxBuffer[BUFFER_SIZE];
uint8_t * xxx = "123456789123456789";
/*
 uint8_t cfgPrepare() {
 uint8_t status;

 BSP_QSPI_Init_t init;
 init.InterfaceMode = MT25TL01G_QPI_MODE;
 init.TransferRate = MT25TL01G_DTR_TRANSFER;
 init.DualFlashMode = MT25TL01G_DUALFLASH_ENABLE;
 status = BSP_QSPI_Init(0, &init);

 if (status != BSP_ERROR_NONE) {
 return CFG_ERR;
 }

 BSP_QSPI_Info_t pQSPI_Info;
 pQSPI_Info.FlashSize = (uint32_t) 0x00;
 pQSPI_Info.EraseSectorSize = (uint32_t) 0x00;
 pQSPI_Info.EraseSectorsNumber = (uint32_t) 0x00;
 pQSPI_Info.ProgPageSize = (uint32_t) 0x00;
 pQSPI_Info.ProgPagesNumber = (uint32_t) 0x00;

 BSP_QSPI_GetInfo(0, &pQSPI_Info);

 if ((pQSPI_Info.FlashSize != 0x8000000)
 || (pQSPI_Info.EraseSectorSize != 0x2000)
 || (pQSPI_Info.ProgPageSize != 0x100)
 || (pQSPI_Info.EraseSectorsNumber != 0x4000)
 || (pQSPI_Info.ProgPagesNumber != 0x80000)) {
 return CFG_ERR;
 }

 return CFG_OK;
 }
 */

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

	uint8_t *start = READ_ADDR;

	if (*(READ_ADDR ) != MAGIC_NUMBER) {
		return CFG_ERR;
	}

	uint32_t bytes_written = *(uint32_t*) (READ_ADDR + 1);

	pb_istream_t stream = pb_istream_from_buffer(READ_ADDR + 5, bytes_written);
	bool o = pb_decode(&stream, my_Configuration_fields, &sys_cfg);

	if (o == false) {
		return CFG_ERR;
	}

	return CFG_OK;
}

uint8_t cfgWrite() {
	*qspi_aTxBuffer = (uint8_t) (0xAA);

	pb_ostream_t stream = pb_ostream_from_buffer((qspi_aTxBuffer + 5),
			sizeof(qspi_aTxBuffer));
	bool o = pb_encode(&stream, my_Configuration_fields, &sys_cfg);

	if (o == false) {
		return CFG_ERR;
	}

	*(qspi_aTxBuffer + 1) = stream.bytes_written;

	uint8_t status;
	status = cfgDisableMemoryMappedMode();
	if (status != BSP_ERROR_NONE) {
		return CFG_ERR;
	}

	status = BSP_QSPI_EraseBlock(0, WRITE_ADDR, BSP_QSPI_ERASE_8K);
	if (status != BSP_ERROR_NONE) {
		return CFG_ERR;
	}

	status = BSP_QSPI_Write(0, xxx, WRITE_ADDR, BUFFER_SIZE);
	if (status != BSP_ERROR_NONE) {
		return CFG_ERR;
	}

	status = cfgEnableMemoryMappedMode();
	if (status != BSP_ERROR_NONE) {
		return CFG_ERR;
	}

	status = cfgRead();
	if (status != BSP_ERROR_NONE) {
		return CFG_ERR;
	}

	return CFG_OK;
}
