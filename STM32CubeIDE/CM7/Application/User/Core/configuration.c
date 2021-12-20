#include "configuration.h"

#include <pb.h>
#include <pb_decode.h>
#include <pb_encode.h>

my_Configuration sys_cfg = my_Configuration_init_zero;

uint8_t cfgRead() {
	/*
	FRESULT res;
	res = cfgEnsureMount();
	if (res != FR_OK) {
		return res;
	}
	FIL fp;
	UINT size;
	pb_istream_t stream;
	res = f_open(&fp, filename, FA_OPEN_ALWAYS | FA_READ);
	if (res != FR_OK) {
		return res;
	}
	res = f_read(&fp, buffer, sizeof(buffer), &size);
	if (res != FR_OK) {
		return res;
	}
	stream = pb_istream_from_buffer(buffer, sizeof(buffer));
	bool o = pb_decode(&stream, my_Configuration_fields, &sys_cfg);
	*/
	return 0;
}

uint8_t cfgWrite() {
	/*
	FRESULT res = cfgEnsureMount();
	if (res != FR_OK) {
		return res;
	}
	FIL fp;
	UINT size;
	res = f_open(&fp, filename, FA_OPEN_ALWAYS | FA_WRITE);
	pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));
	bool o = pb_encode(&stream, my_Configuration_fields, &sys_cfg);
	res = f_write(&fp, buffer, stream.bytes_written, &size);
	res = f_sync(&fp);
	res = f_close(&fp);
	*/
	return 0;
}
