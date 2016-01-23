#include "../src/iface.h"

#include <map>
#include <cmath>
#include "zlib.h"

using namespace std;

plugin_t *init(){
	plugin_t *self = (plugin_t*) calloc(1, sizeof(plugin_t));
	self->name = "cwf";
	self->description = "Dumps CWF files (zlib compressed SWF)";

	return self;
}

/*********************************************************************************************
| HEADER (bytes)
|
| 1 | 1 | 1 | 1 | 4 | 1 | 1 | 4
| |   |   |   |   |   |   |   |-> DICTID (Only when FLG is set)
| |   |   |   |   |   |   |
| |   |   |   |   |   |   |-> FLG (Flags, FCHECK == 5 bits, FDICT == 1 bit, FLEVEL == 2 bits)
| |   |   |   |   |   |
| |   |   |   |   |   |-> CMF (Compression Method and Flags, CM == 4 bits, CINFO == 4 bits)
| |   |   |   |   |
| |   |   |   |   |-> length of file in bytes (doesn't match if compressed)
| |   |   |   |
| |   |   |   |-> version as bit, not as ASCII (v4 == 0x04, not 0x34)
| |   |   |
| |   |   |-> signature (always 'S')
| |   |
| |   |-> signature (always 'W')
| |
| |-> signature ('C', zlib compressed, only after v6+)
|
| http://wwwimages.adobe.com/www.adobe.com/content/dam/Adobe/en/devnet/swf/pdf/swf-file-format-spec.pdf
|
*********************************************************************************************/

void process(Bits *data){

	while(true){

		size_t start = data->findNext("CWS", 3);
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

		/*Only Flash version 6+ supports zlib compression*/
		if(s_version < 6) {
			continue;
		}
		/*Read Flash file size*/
		uint32_t size = data->read_uint32(true);

		/*Read CMF*/
		uint8_t cmf = data->read_uint8();

		/*Read CM and CINFO*/
		if((cmf & 0x0F) != 8 || (cmf & 0xF0) < 7) {
			continue;
		}

		uint8_t flg = data->read_uint8();
		uint8_t fcheck = flg & 0x1F;

		data->seek(2, true);
		uint16_t check = data->read_uint16();
		if(check % 31 != 0) {
			continue;
		}

		/*At this point we probably have a valid DEFLATE data*/
		data->seek(2, true);

		/*Zlib magic*/
		void *dst[size];

		z_stream strm  = {0};
		strm.total_in  = strm.avail_in  = data->getMaxPosition() - data->getPosition();
		strm.total_out = strm.avail_out = size;
		strm.next_in   = (Bytef *) (data->getData() + data->getPosition());
		strm.next_out  = (Bytef *) dst;

		strm.zalloc = Z_NULL;
		strm.zfree  = Z_NULL;
		strm.opaque = Z_NULL;

		//+32 tells zlib to to detect if using gzip or zlib
		if (inflateInit2(&strm, (MAX_WBITS + 32)) == Z_OK) {
			if (inflate(&strm, Z_FINISH) != Z_STREAM_END) {
				 inflateEnd(&strm);
				 continue;
			}
		} else {
			inflateEnd(&strm);
			continue;
		}

		inflateEnd(&strm);
		size_t cur_offset = data->getPosition() - start;
		size_t decompressed_size = strm.total_out + cur_offset;
		size_t compressed_size = strm.total_in + cur_offset;
		if(decompressed_size != size) {
			continue;
		}

		cout << "FWS (CWS) match (" << compressed_size / 1024 << " kb (" << compressed_size << " bytes)" <<
				", version " << s_version << ")\n";

		data->toRandFile("./dumps/", "swf", start, compressed_size);
	}

}
