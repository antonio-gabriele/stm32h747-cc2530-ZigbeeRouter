#include "system.h"

#include <stm32h747i_discovery_qspi.h>
#include <application.h>
#include <fatfs.h>

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

void init() {
    FATFS fs;
    FRESULT res;
    BYTE work[_MAX_SS]; // Formats the drive if it has yet to be formatted
    //res = f_mkfs("0:", FM_FAT32, 0, work, sizeof work);
    //if(res != FR_OK) {
    //    return;
    //}
    res = f_mount(&fs, "0:", 1);
    if(res != FR_OK) {
        return;
    }

/*
    uint32_t freeClust;
    FATFS* fs_ptr = &fs;
    res = f_getfree("", &freeClust, &fs_ptr); // Warning! This fills fs.n_fatent and fs.csize!
    if(res != FR_OK) {
        return;
    }


    uint32_t totalBlocks = (fs.n_fatent - 2) * fs.csize;
    uint32_t freeBlocks = freeClust * fs.csize;


    DIR dir;
    res = f_opendir(&dir, "/");
    if(res != FR_OK) {
        return;
    }

    FILINFO fileInfo;
    uint32_t totalFiles = 0;
    uint32_t totalDirs = 0;
    for(;;) {
        res = f_readdir(&dir, &fileInfo);
        if((res != FR_OK) || (fileInfo.fname[0] == '\0')) {
            break;
        }

        if(fileInfo.fattrib & AM_DIR) {
            totalDirs++;
        } else {
            totalFiles++;
        }
    }


    res = f_closedir(&dir);
    if(res != FR_OK) {
        return;
    }
*/

    char writeBuff[128];
    snprintf(writeBuff, sizeof(writeBuff), "Total blocks: %lu (%lu Mb); Free blocks: %lu (%lu Mb)\r\n",
        0,
        0);

    FIL logFile;
    res = f_open(&logFile, "log.txt", FA_OPEN_APPEND | FA_WRITE);
    if(res != FR_OK) {
        return;
    }

    unsigned int bytesToWrite = strlen(writeBuff);
    unsigned int bytesWritten;
    res = f_write(&logFile, writeBuff, bytesToWrite, &bytesWritten);
    if(res != FR_OK) {
        return;
    }

    if(bytesWritten < bytesToWrite) {
    }

    res = f_close(&logFile);
    if(res != FR_OK) {
        return;
    }

    FIL msgFile;
    res = f_open(&msgFile, "log.txt", FA_READ);
    if(res != FR_OK) {
        return;
    }

    char readBuff[128];
    unsigned int bytesRead;
    res = f_read(&msgFile, readBuff, sizeof(readBuff)-1, &bytesRead);
    if(res != FR_OK) {
        return;
    }

    readBuff[bytesRead] = '\0';

    res = f_close(&msgFile);
    if(res != FR_OK) {
        return;
    }

    // Unmount
    res = f_mount(NULL, "", 0);
    if(res != FR_OK) {
        return;
    }

}

uint8_t cfgRead() {
	init();
	/*
	uint8_t *src = (uint8_t*) (READ_ADDR);
	memcpy(&sys_cfg, src, sizeof(Configuration_t));
	if(sys_cfg.MagicNumber != MAGIC_NUMBER){
		memset(&sys_cfg, 0, sizeof(Configuration_t));
		cfgWrite();
	}
	*/
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
