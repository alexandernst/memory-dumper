#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#endif
#define _FILE_OFFSET_BITS 64

#include <vector>
#include <iostream>
#include <cstdlib>

#include <errno.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <signal.h>
#include <unistd.h>
#include <limits.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ptrace.h>

#include "iface.h"

#define PLUGINS_DIR "./plugins/"

class MemoryDumper {

	public:
		bool from_file = false, from_process = false;
		std::vector<Bits *> *chunks;
		std::vector<plugin_t *> *plugins;

		MemoryDumper();
		virtual ~MemoryDumper();

		bool init(int pid);
		bool init(char *file);
		bool initPlugins(char *plugins_list);
		bool getChunksFromFile();
		bool getChunksFromProcess();

	private:
		int pid;
		char *file;
		bool keep_running;

};
