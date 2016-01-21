#ifndef BITS_H
#define	BITS_H

#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#endif

#include <ios>
#include <cmath>
#include <bitset>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <cinttypes>
#include "Utils.h"

#undef get16bits
#if (defined(__GNUC__) && defined(__i386__)) || defined(__WATCOMC__) || defined(_MSC_VER) || defined (__BORLANDC__) || defined (__TURBOC__)
#define get16bits(d) (*((const uint16_t *) (d)))
#endif

#if !defined (get16bits)
#define get16bits(d) ((((uint32_t)(((const uint8_t *)(d))[1])) << 8) + (uint32_t)(((const uint8_t *)(d))[0]) )
#endif

class Bits {

	public:

		Bits(const std::string& fname = "", std::ios_base::openmode mode = std::ios::in | std::ios::binary);

		Bits(unsigned char *chunk, size_t size);

		Bits(unsigned char *chunk, size_t size, bool auto_free_mem);

		virtual ~Bits();

		void autoFreeMem(bool auto_free_mem);

		bool canMoveBackwards(size_t n_bytes = 1);

		bool canMoveForward(size_t n_bytes = 1);

		bool checkIfError();

		bool toFile(const std::string& fname = "", size_t offset = 0, size_t size = 0, std::ios_base::openmode mode = std::ios::out | std::ios::binary | std::ios::trunc);

		bool toRandFile(const std::string& dir = "./", const std::string& ext = "", size_t offset = 0, size_t size = 0, std::ios_base::openmode mode = std::ios::out | std::ios::binary | std::ios::trunc);

		void clear();

		void unload();

		unsigned char *read(size_t n, bool reverse = false);

		uint8_t read_uint8();

		uint16_t read_uint16(bool reverse = false);

		uint32_t read_uint32(bool reverse = false);

		uint64_t read_uint64(bool reverse = false);

		Bits *readBits(size_t n_bits, size_t skip_n_bits = 0);

		bool compareBinary(const std::string& str, size_t check_n_bits, size_t skip_b_bits = 0);

		bool compareHex(const std::string& str, size_t check_n_bytes, size_t skip_b_bytes = 0);

		unsigned char *peek(size_t n, bool reverse = false);

		bool write(unsigned char *chunk, size_t n, bool patch = true);

		bool seek(size_t n, bool reverse = false);

		size_t findPrevious(const std::string& pattern, size_t n);

		size_t findNext(const std::string& pattern, size_t n);

		bool testBit(unsigned int bit);

		bool setBit(unsigned int bit);

		bool unsetBit(unsigned int bit);

		bool toggleBit(unsigned int bit);

		uint32_t getHash();

		void printHash();

		unsigned char *getAsHex(size_t num_bytes);

		void printAsHex(size_t num_bytes);

		unsigned char *getAsBinary(size_t num_bytes);

		void printAsBinary(size_t n);

		unsigned char *getData();

		size_t getPosition();

		bool setPosition(size_t pos);

		size_t getMaxPosition();

	private:

		uint32_t hash;
		unsigned char *data;
		size_t position, max_position;
		bool auto_free_mem, is_from_file, error;

		void init();
		bool fromFile(const std::string& fname = "", std::ios_base::openmode mode = std::ios::in | std::ios::binary);
		bool fromMem(unsigned char *chunk, size_t size);

		void unsetError();
		void setError();

};

#endif	/* BITS_H */
