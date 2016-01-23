#include "../src/iface.h"

#include <map>

using namespace std;

plugin_t *init(){
	plugin_t *self = (plugin_t*) calloc(1, sizeof(plugin_t));
	self->name = "swf";
	self->description = "Dumps SWF files (No support for LZMA currently)";

	return self;
}

/*********************************************************************************************
| HEADER (bytes)
|
| 1 | 1 | 1 | 1 | 4 | X | 2 | 2
| |   |   |   |   |   |   |   |-> frame count
| |   |   |   |   |   |   |
| |   |   |   |   |   |   |-> frame rate (ignore first byte)
| |   |   |   |   |   |
| |   |   |   |   |   |-> frame size (variable length)
| |   |   |   |   |
| |   |   |   |   |-> length of file in bytes (doesn't match if compressed)
| |   |   |   |
| |   |   |   |-> version as bit, not as ASCII (v4 == 0x04, not 0x34)
| |   |   |
| |   |   |-> signature (always 'S')
| |   |
| |   |-> signature (always 'W')
| |
| |-> signature ('F' == uncompressed, 'C' == zlib only after v6+, 'Z' == LZMA only after v13+)
|
| http://wwwimages.adobe.com/www.adobe.com/content/dam/Adobe/en/devnet/swf/pdf/swf-file-format-spec.pdf
|
*********************************************************************************************/

void process(Bits *data){

	while(true){ /*TODO: Support CWF and ZWF ?*/

		size_t start = data->findNext("FWS", 3);
		if(data->checkIfError() == true){
			break;
		}

		data->setPosition(start);
		if(data->canMoveForward(21) == false){
			break;
		}

		/*Reset the size to 0 after each loop*/
		size_t total_length = 0;

		/*Skip 3 bytes because of the signature*/
		data->setPosition(start + 3);
		total_length += 3;

		/*Read Flash version*/
		uint8_t version = data->read_uint8();
		total_length += 1;

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

		/*Read Flash file size*/
		uint32_t size = data->read_uint32(true);
		total_length += 4;

		/*Check if we can actually read as many bytes as the header claims*/
		if(!data->canMoveForward(size - total_length)){
			continue;
		}

		/*Skip RECT values, we don't care about those*/
		uint8_t bits_per_field = data->read_uint8();
		bits_per_field = (uint8_t)bits_per_field >> 3;
		uint8_t total_bits = 5 + bits_per_field * 4;
		total_bits = ((total_bits) % 8) ? (total_bits / 8 + 1) : total_bits / 8;
		data->seek(total_bits - 1); //we started by reading 1 byte to determine the size of the RECT
		total_length += total_bits;

		//uint32_t rect = data->read_uint32();

		/*Read Flash frame rate and frame count*/
		data->seek(1);
		uint16_t framerate = (uint16_t)data->read_uint8();
		uint16_t framecount = data->read_uint16(true);
		total_length += 4;

		/*Framerate or framecount equal to 0. Probably a false match.*/
		if(framerate == 0 || framecount == 0){
			continue;
		}

		/*Read each frame and decide if we have a valid file*/
		while(true){
			uint16_t tag = data->read_uint16(true);
			uint16_t tag_type = tag >> 6;
			uint32_t tag_length = tag & 0x3F;

			total_length += 2;
			if(tag_length >= 63){
				tag_length = (int32_t)data->read_uint32(true);
				total_length += 4;
			}
			total_length += tag_length;

			data->seek(tag_length);

			//Perfect place to scan all tag types and extract fonts and pictures
			if(tag_type == 0 || total_length >= size){
				break;
			}
		}

		if(total_length != size){
			continue;
		}

		cout << "FWS match (" << size / 1024 << " kb (" << size << " bytes)" <<
				", version " << s_version <<
				", framerate: " << framerate <<
				", framecount: " << framecount << ")\n";

		data->seek(-size);

		data->toRandFile("./dumps/", "swf", start, size);
	}

}
