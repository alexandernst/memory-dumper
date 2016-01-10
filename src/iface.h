#pragma once
#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64

#include <cstdlib>
#include <string>
#include <iostream>

typedef struct plugin_t {
	void *hndl;
	std::string name;
	std::string description;
} plugin_t;

#include "../libs/cpp-bitstring/Bits.h"
extern "C" plugin_t *init();
extern "C" void process(Bits *data);
