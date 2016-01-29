#include "../src/iface.h"
#include "zlib.h"

using namespace std;

plugin_t *init(){
	plugin_t *self = new plugin_t();
	self->name = "png";
	self->description = "Dumps PNG images";

	return self;
}

/*********************************************************************************************
| HEADER (bytes)
|
| 8
| |
| |-> signature ('"\x89"PNG-')
|
| http://www.libpng.org/pub/png/spec/1.2/PNG-Structure.html
|
*********************************************************************************************/

void process(Bits *data){
	while(true){

		size_t start = data->findNext("\x89PNG\x0D\x0A\x1A\x0A", 8);
		if(data->checkIfError() == true){
			break;
		}

		// The minimum size of a PNG image is 44 bytes. If we can move forward 44 bytes in memory, this match probably is a PNG. If not... out.
		data->setPosition(start);
		if(data->canMoveForward(44) == false){
			break;
		}

		// Check if the first chunk forward the header type is IHDR, if not, out.
		data->seek(12);
		if(data->findNext("IHDR", 4) != data->getPosition() || data->checkIfError() == true){
			continue;
		}

		// Get back to the end of the header
		data->seek(4, true);

		// Read IHDR size
		uint32_t IHDR_size = data->read_uint32();

		// Check if IHDR size is correct
		if(IHDR_size != 13){
			continue;
		}

		//Generate a CRC
		uint32_t crc = crc32(0, (const Bytef*) data->getData() + data->getPosition(), 4 + IHDR_size);

		data->seek(4 + IHDR_size);

		// Read CRC
		uint32_t IHDR_crc32 = data->read_uint32();

		// Check if both CRC are the same
		if(crc != IHDR_crc32){
			continue;
		}

		// Get back at the beginning of the data segment
		data->seek(IHDR_size + 4, true);

		// Read the first 4 bytes of the data segment (Width)
		uint32_t width = data->read_uint32();

		// Read the next 4 bytes of the data semgent (Height)
		uint32_t height = data->read_uint32();

		// Read the next byte of the data segment (Bit depth)
		uint8_t bit_depth = data->read_uint8();

		// Read the next byte of the data segment (Color type)
		uint8_t color_type = data->read_uint8();

		// Check if Color type and Bit depth restrictions are valid
		int bdc_check[][5] = {
			{1, 2, 4, 8, 16},
			{},
			{8, 16},
			{1, 2, 4, 8},
			{8, 16},
			{},
			{8, 16}
		};

		if(find(begin(bdc_check[color_type]), end(bdc_check[color_type]), bit_depth) == end(bdc_check[color_type])){
			continue;
		}

		bool plte_must_not_exist = false;
		if(color_type == 0 || color_type == 4){
			plte_must_not_exist = true;
		}

		bool plte_is_required = false;
		if(color_type == 3){
			plte_is_required = true;
		}

		bool plte_is_found = false;
		bool iend_is_found = false;

		/*
		// Read the next byte of the data segment (Compression method)
		uint8_t compression_method = data->read_uint8();

		// Read the next byte of the data segment (Filter method)
		uint8_t filter_method = data->read_uint8();

		// Read the next byte of the data segment (Interface method)
		uint8_t interface_method = data->read_uint8();
		*/
		data->seek(3);

		// Move to the end of IHDR chunk
		data->seek(4);

		size_t end = 0;
		size_t size = 0;

		// Read all the chunks
		while(iend_is_found == false){
			// Check if there is enough memory
			if(data->canMoveForward(8) == false){
				goto fatal_err;
			}

			uint32_t tmp_size = data->read_uint32();
			uint32_t tmp_type = data->read_uint32();

			// Check if there is enough memory
			if(data->canMoveForward(tmp_size + 4) == false){
				goto fatal_err;
			}

			data->seek(4, true);

			//Generate a CRC
			uint32_t tmp_tmp_crc = crc32(0, (const Bytef*) data->getData() + data->getPosition(), 4 + tmp_size);

			data->seek(4 + tmp_size);

			// Read CRC
			uint32_t tmp_crc = data->read_uint32();

			// Check if both CRC are the same
			if(tmp_tmp_crc != tmp_crc){
				goto no_fatal_err;
			}

			// Check if this chunk is a PLTE chunk
			if(tmp_type == 0x504C5445){
				plte_is_found = true;
				if(plte_must_not_exist){
					goto no_fatal_err;
				}
			}

			//Check if this chunk is the IEND chunk
			if(tmp_type == 0x49454E44){
				iend_is_found = true;
				end = data->getPosition();
				break;
			}
		}

		if(plte_is_found == false && plte_is_required == true){
			continue;
		}

		size = end - start;

		cout << "PNG match (" << size / 1024 << " kb (" << size << " bytes)" <<
				", width " << width <<
				", height: " << height << ")\n";

		// Save the image
		data->toRandFile("./dumps/", "png", start, size);

		// Handle inner while loop break
		no_fatal_err:
			continue;

		fatal_err:
			break;
	}

}
