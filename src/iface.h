#pragma once
#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64

#include <string>

using namespace std;

typedef struct plugin_t {
	void *hndl;
	string name;
	string description;
} plugin_t;

#include "../libs/cpp-bitstring/Bits.h"
extern "C" plugin_t *init();
extern "C" void process(Bits *data);
