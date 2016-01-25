#include "../src/iface.h"

#include <map>
#include <cmath>
#include "lzma/LzmaDec.c"

using namespace std;

plugin_t *init(){
	plugin_t *self = new plugin_t();
	self->name = "zwf";
	self->description = "Dumps ZWF files (LZMA compressed SWF)";

	return self;
}

/*********************************************************************************************
| HEADER (bytes)
|
| 1 | 1 | 1 | 1 | 4 | 4 | 5
| |   |   |   |   |   |   |-> LZMA properties
| |   |   |   |   |   |
| |   |   |   |   |   |-> length of compressed file in bytes
| |   |   |   |   |
| |   |   |   |   |-> length of uncompressed file in bytes
| |   |   |   |
| |   |   |   |-> version as bit, not as ASCII (v4 == 0x04, not 0x34)
| |   |   |
| |   |   |-> signature (always 'S')
| |   |
| |   |-> signature (always 'W')
| |
| |-> signature ('Z', zlib compressed, only after v13+)
|
| http://wwwimages.adobe.com/www.adobe.com/content/dam/Adobe/en/devnet/swf/pdf/swf-file-format-spec.pdf
|
*********************************************************************************************/

static void *SzAlloc(void *p, size_t size) { p = p; return malloc(size); }
static void SzFree(void *p, void *address) { p = p; free(address); }
static ISzAlloc g_Alloc = { SzAlloc, SzFree };

void process(Bits *data){

	while(true){

		size_t start = data->findNext("ZWS", 3);
		if(data->checkIfError() == true){
			break;
		}

		data->setPosition(start);
		if(data->canMoveForward(10) == false){
			break;
		}

		/*Skip 3 bytes because of the signature*/
		data->setPosition(start + 3);

		/*Read Flash version*/
		uint8_t version = data->read_uint8();

		/*Find version*/
		map<int, float> versions {
			//http://www.adobe.com/devnet/articles/flashplayer-air-feature-list.html
			{0x01, 1}, {0x02, 2}, {0x03, 3}, {0x04, 4}, {0x05, 5}, {0x06, 6}, {0x07, 7}, {0x08, 8}, {0x09, 9},
			{0x0A, 10.0}, {0x0B, 10.2}, {0x0C, 10.3}, {0x0D, 11.0}, {0x0E, 11.1}, {0x0F, 11.2}, {0x10, 11.3},
			{0x11, 11.4}, {0x12, 11.5}, {0x13, 11.6}, {0x14, 11.7}, {0x15, 11.8}, {0x16, 11.9}, {0x17, 12.0},
			{0x18, 13.0}, {0x19, 14.0}, {0x1A, 15.0}, {0x1B, 16.0}
		};
		map<int, float>::iterator it = versions.begin();
		float s_version = (it = versions.find(version)) != versions.end() ? it->second : 0;

		/*Only Flash version 13+ supports lzma compression*/
		//if(s_version < 13) {
		//	continue;
		//}
		/*Read Flash file size*/
		uint32_t size = data->read_uint32(true);

		/*Read Flash compressed file size*/
		uint32_t csize = data->read_uint32(true);

		/*We should move forward as much as the reported compressed file size*/
		data->seek(5);
		if(!data->canMoveForward(csize)) {
			continue;
		}

		/*LZMA magic*/
		void *dst[size];

		SizeT dst_size_pure = size;
		SizeT src_size_pure = csize;
		ELzmaStatus status;

		int res = LzmaDecode(
			(Byte *) dst, //dest
			&dst_size_pure, //dest size
			data->getData() + data->getPosition(), //src
			&src_size_pure, //src size
			data->getData() + data->getPosition() - 5, //props
			LZMA_PROPS_SIZE, //props size
			LZMA_FINISH_ANY, //mode
			&status, //status
			&g_Alloc //alloc functions
		);

		if (res != SZ_OK) {
			continue;
		}

		cout << "FWS (CWS) match (" << csize / 1024 << " kb (" << csize << " bytes)" <<
				", version " << s_version << ")\n";

		data->toRandFile("./dumps/", "swf", start, csize);
	}

}
