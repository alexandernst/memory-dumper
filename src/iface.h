#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#endif
#define _FILE_OFFSET_BITS 64

#ifndef IFACE_H_INCLUDED
#define IFACE_H_INCLUDED

using namespace std;

typedef struct plugin_t {
	void *hndl;
	string name;
	string description;
} plugin_t;

#include "libs/cpp-bitstring/Bits.h"
extern "C" plugin_t *init();
extern "C" void process(Bits *data);

#endif
