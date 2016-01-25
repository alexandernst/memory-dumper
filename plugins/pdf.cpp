#include "../src/iface.h"

#include <map>

using namespace std;

plugin_t *init(){
	plugin_t *self = new plugin_t();
	self->name = "pdf";
	self->description = "Dumps PDF files (No support for linearized PDF files)";

	return self;
}

/*********************************************************************************************
| HEADER (bytes)
|
| 5 | 3
| |   |
| |   |-> version ('1.0' to '1.7')
| |
| |-> signature ('%PDF-')
|
| http://www.adobe.com/content/dam/Adobe/en/devnet/acrobat/pdfs/PDF32000_2008.pdf
|
*********************************************************************************************/

void process(Bits *data){

	while(true){

		size_t start = data->findNext("%PDF-", 5);
		if(data->checkIfError() == true){
			break;
		}

		data->setPosition(start);
		/*291 bytes seems to be to smallest possible (valid) PDF size*/
		/* http://stackoverflow.com/questions/17279712/what-is-the-smallest-possible-valid-pdf */
		if(data->canMoveForward(291) == false){
			break;
		}

		data->setPosition(start + 5);
		string s_version((char *) data->read(3));

		/*Try to find the end of the PDF file*/
		size_t eof = data->findNext("%%EOF", 5);
		if(data->checkIfError() == true){
			break;
		}

		/*Set the possible size of the PDF fize*/
		size_t size = (eof + 5) - start;

		/*Check if we're dealing with a real PDF file or if we just found a false positive match*/
		data->setPosition(eof);
		size_t startxref = data->findPrevious("startxref", 9);

		/*The gap between the EOF and the 'startxref' shouldn't be bigger than ~20bytes*/
		if(eof - (startxref + 9) > 20){
			data->setPosition(eof + 5);
			continue;
		}

		data->setPosition(startxref + 9);
		string offset_s( (char *) data->read(eof - data->getPosition()));
		offset_s = Utils::trim(Utils::removeSpaces(offset_s));
		size_t offset = atoi(offset_s.c_str());

		if(offset < start){
			data->setPosition(eof + 5);
			continue;
		}

		data->setPosition(start + offset - 1);

		size_t xref = data->findNext("xref", 4);
		if( xref != start + offset || data->checkIfError() == true){
			data->setPosition(eof + 5);
			continue;
		}

		cout << "PDF match (" << size / 1024 << " kb (" << size << " bytes)" <<
				", version " << s_version << ")\n";

		data->toRandFile("./dumps/", "pdf", start, size);
	}

}
