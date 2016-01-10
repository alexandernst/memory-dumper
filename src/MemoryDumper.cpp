#include "MemoryDumper.h"
using namespace std;

MemoryDumper::MemoryDumper(){
	this->chunks = new std::vector<Bits *>;
	this->plugins = new std::vector<plugin_t *>;
}

MemoryDumper::~MemoryDumper(){
	for(std::vector<struct plugin_t *>::iterator plugins_iter = this->plugins->begin(); plugins_iter != this->plugins->end(); ++plugins_iter){
		dlclose((*plugins_iter)->hndl);
		free((*plugins_iter));
	}

	delete this->chunks;
	delete this->plugins;
}

bool MemoryDumper::init(char *file){
	this->file = (char *) calloc(1, PATH_MAX+1);
	this->from_file = true;

	/*Get full path of file*/
	char *p = realpath(file, this->file);
	if(p == NULL){
		return false;
	}

	return true;
}

bool MemoryDumper::init(int pid){
	this->pid = pid;
	this->from_process = true;

	/*Try to attach to the process*/
	if(ptrace(PTRACE_ATTACH, this->pid, NULL, NULL) == -1){
		printf("Error attaching to process.\n");
		return false;
	}
	waitpid(this->pid, NULL, 0);

	/*Dettach from the process*/
	ptrace(PTRACE_DETACH, this->pid, NULL, NULL);

	return true;
}

bool MemoryDumper::initPlugins(char *plugins_list){
	DIR *dir;
	char *error;
	struct dirent *ent;

	/*Open the plugins folder...*/
	if((dir = opendir(PLUGINS_DIR)) == NULL){
		printf("Can't open plugins dir\n");
		return EXIT_FAILURE;
	}

	/*... and read it's content*/
	while((ent = readdir(dir)) != NULL){
		/*Make vars shorter*/
		char *fname = ent->d_name;
		int fnlen = strlen(fname);

		/*If the file name is shorter than 3 chars or it doesn't end with '.so'*/
		/*then it's not a possible plugin, so just skip it*/
		if(fnlen <= 3 || strcmp(fname + fnlen - 3, ".so") != 0){
			continue;
		}

		printf("Loading %s... ", fname);

		/*Allocate some space for the path to the plugin, NULL-terminated*/
		char *lib_path = (char *) calloc(1, strlen(PLUGINS_DIR) + fnlen + 1);
		sprintf(lib_path, "%s%s", PLUGINS_DIR, fname);

		/*Load the plugin*/
		void *plugin_handle = dlopen(lib_path, RTLD_NOW);
		free(lib_path);

		/*Error trying to load the .so file we found. Maybe not a plugin?*/
		/*Keep going anyways...*/
		if(!plugin_handle){
			printf("Error loading plugin!\n%s\n", dlerror());
			continue;
		}

		/*Init the plugin by calling it's 'init' function*/
		plugin_t *(*initf)();
		*(plugin_t **) (&initf) = (plugin_t *) dlsym(plugin_handle, "init\0");

		if((error = dlerror()) != NULL){
			/*Oops... something wen't wrong calling the plugin's init function*/
			printf("Error loading init function!\n%s\n", error);
			continue;
		}

		printf("Ok\n");

		/*Everything went fine, lets create/update the linked list containing all the loaded plugins*/
		plugin_t *plugin = (*initf)();
		plugin->hndl = plugin_handle;
		this->plugins->push_back(plugin);

		printf("\t%s - %s\n", plugin->name, plugin->description);

		/*Check if we should actually load the plugin*/
		if(plugins_list != NULL){
			printf("\tPlugin does not match with load list, unloading %s...\n", plugin->name);
			/*Close handle and free memory*/
			dlclose(plugin->hndl);
			free(plugin);
		}
	}

	/*We are done, close the plugins folder*/
	closedir(dir);

	return true;
}

bool MemoryDumper::getChunksFromFile(){
	Bits *f = new Bits(this->file);
	this->chunks->push_back(f);

	return true;
}

bool MemoryDumper::getChunksFromProcess(){
	off64_t start, end;
	unsigned long int inode, foo;
	char path[BUFSIZ], line_buf[BUFSIZ + 1], mapname[PATH_MAX], perm[5], dev[6];

	/*Attach to the process*/
	if(ptrace(PTRACE_ATTACH, this->pid, NULL, NULL) == -1){
		printf("Error attaching to process.\n");
		return EXIT_FAILURE;
	}
	waitpid(this->pid, NULL, 0);

	/*Open the fds from where we're going to read*/
	snprintf(path, sizeof(path), "/proc/%d/maps", this->pid);
	FILE *maps_fd = fopen(path, "r");
	snprintf(path, sizeof(path), "/proc/%d/mem", this->pid);
	int mem_fd = open(path, O_RDONLY);

	if(!maps_fd || mem_fd == -1){
		printf("Error opening /proc/%d/maps or /proc/%d/mem\n", this->pid, this->pid);
		return false;
	}

	/*We're ready to read*/
	while(fgets(line_buf, BUFSIZ, maps_fd) != NULL){

		/*Quit faster if we receive Ctrl-c*/
		//if(!this->keep_running){
		//	break;
		//}

		mapname[0] = '\0';
		sscanf(line_buf, "%lx-%lx %4s %lx %5s %ld %s", &start, &end, perm, &foo, dev, &inode, mapname);
		/*We don't want to read the memory of shared libraries, devices, etc...*/
		if(foo != 0 || inode != 0 || access(mapname, R_OK) == 0){
			continue;
		}
		/*Avoid non-sense*/
		if(start > end){
			continue;
		}

		size_t mem_size = end - start;
		unsigned char *mem_buf = (unsigned char *) malloc(mem_size);
		if(!mem_buf){
			printf("Error allocating space!\n");
			continue;
		}

		lseek64(mem_fd, start, SEEK_SET);
		if(read(mem_fd, mem_buf, mem_size) == -1){
			printf("Error copying raw memory! (%s)\n", strerror(errno));
			free(mem_buf);
			continue;
		}

		/*Push a Bits object to the chunks vector so we can then use it from the plugins*/
		Bits *b = new Bits(mem_buf, mem_size, true);
		this->chunks->push_back(b);
	}

	/*Dettach from the process*/
	ptrace(PTRACE_DETACH, this->pid, NULL, NULL);

	/*Close file descriptors*/
	mem_fd != -1 && close(mem_fd);
	maps_fd && fclose(maps_fd);

	return true;
}

int main(int argc, char **argv){
	char *plugins_list = NULL;
	int ch;
	bool only_show_plugins = false;
	MemoryDumper *md = new MemoryDumper();

	/*Get all those args!*/
	while((ch = getopt(argc, argv, "f:p:l:sh")) != -1){
		switch(ch){
			case 'f':
				if(!md->init(optarg)){
					//return fail;
				}
			case 'p':
				if(!md->init(atoi(optarg))){
					//return fail;
				}
				break;
			case 'l':
				plugins_list = optarg;
				break;
			case 's':
				only_show_plugins = true;
				break;
			case 'h':
				printf("Available options:\n");
				printf("\tp <pid> - PID of the process you want to dump\n");
				printf("\tl <list,of,plugins | all> - Load only certain plugins, comma separated\n");
				printf("\ts - Show available plugins\n");
				printf("\th - Show this help\n");
		}
	}

	if(only_show_plugins){
		printf("Available plugins:\n");
	}

	md->initPlugins(plugins_list);

	if(only_show_plugins){
		delete md;
		return EXIT_SUCCESS;
	}

	if(md->from_file){
		md->getChunksFromFile();
	}else if(md->from_process){
		md->getChunksFromProcess();
	}

	for(std::vector<struct plugin_t *>::iterator plugins_iter = md->plugins->begin(); plugins_iter != md->plugins->end(); ++plugins_iter){

		for(std::vector<Bits *>::iterator chunks_iter = md->chunks->begin(); chunks_iter != md->chunks->end(); ++chunks_iter){
			void *(*f)(Bits *);

			*(void **) (&f) = dlsym((*plugins_iter)->hndl, "process\0");

			(*f)((*chunks_iter));
		}

	}

	delete md;
	return EXIT_SUCCESS;
}
