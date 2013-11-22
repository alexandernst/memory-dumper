#define _LARGEFILE64_SOURCE

#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "../iface.h"

funcs_t *_;

plugin_t *init(funcs_t *funcs){
	_ = funcs;

	plugin_t *this = calloc(1, sizeof(plugin_t));
	this->name = "swf";
	this->description = "Dumps SWF files (No support for Zlib nor LZMA currently)";

	return this;
}

/*********************************************************************************************
| HEADER (bytes)
|
| 1 | 1 | 1 | 1 | 4 | X | 2 | 2 
| |   |   |   |   |   |   |   |-> frame count
| |   |   |   |   |   |   |-> frame rate (ignore first byte)
| |   |   |   |   |   |-> frame size (variable length)
| |   |   |   |   |-> length of file in bytes (doesn't match if compressed)
| |   |   |   |-> version as bit, not as ASCII (v4 == 0x04, not 0x34)
| |   |   |-> signature (always 'S')
| |   |-> signature (always 'W')
| |-> signature ('F' == uncompressed, 'C' == zlib only after v6+, 'Z' == LZMA only after v13+)
|
| http://wwwimages.adobe.com/www.adobe.com/content/dam/Adobe/en/devnet/swf/pdf/swf-file-format-spec.pdf
|
*********************************************************************************************/

void match(unsigned char *mem_buf, unsigned long int mem_size){
	unsigned long int i;
	unsigned char *fcontent, *hash, *str_hash;
	unsigned int size, version, framerate, framecount;

	for(i = 0; i < mem_size - 7; i++){
		if(memcmp(mem_buf + i, "FWS", 3) == 0){ /*TODO: Support CWF and ZWF ?*/
			
			version = mem_buf[i + 3];

			size = mem_buf[i + 4] | (mem_buf[i + 5] << 8) | (mem_buf[i + 6] << 16) | (mem_buf[i + 7] << 24);			

			//Skip 9 bytes (RECT values, we don't care about those)

			framerate = mem_buf[i + 17 + 1];
			framecount = mem_buf[i + 17 + 2] | (mem_buf[i + 17 + 3] << 8);

			/*File size too small/big*/
			if(size < 1 || size > 50 * 1024 * 1024){ /*50mb SWF file... scary*/
				continue;
			}

			/*Framerate or framecount equal to 0. Probably a false match.*/
			if(framerate == 0 || framecount == 0){
				continue;
			}

			printf("FWS match (%u kb, version %u, framerate: %u, framecount: %u)! -> ", size / 1024, version, framerate, framecount);

			fcontent = malloc(size);			
			memcpy(fcontent, mem_buf + i, size);

			hash = _->get_mem_buf_hash(fcontent, size);
			str_hash = _->hash_to_str(hash);

			if(_->is_mem_buf_dumped(hash)){
				printf("Memory block already dumped. Skipping...\n");
			}else{
				char fname[255];
				sprintf(fname, "./dumps/%s-%lu-%lu-%u.swf", str_hash, time(NULL), i, size);

				printf("Dumping to %s!\n", fname);

				_->dump_mem_buf(fcontent, size, fname);
			}

			free(fcontent);
			free(hash);
			free(str_hash);

		}
	}

}
