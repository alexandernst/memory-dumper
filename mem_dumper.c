#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <signal.h>
#include <unistd.h>
#include <limits.h>
#include <sys/ptrace.h>

#include "iface.h"

#define PLUGINS_DIR "./plugins/"

static int keepRunning = 1;

void intHandler(int dummy){
    keepRunning = 0;
}

int main(int argc, char **argv){
	DIR *dir;
	FILE *maps_fd;
	off64_t start, end;
	struct dirent *ent;
	struct sigaction act;
	void *plugin_handle;
	int i, ch, pid, mem_fd, fnlen, show;
	unsigned long int inode, foo, mem_size;
	char path[BUFSIZ], *l, *error, *fname, *lib_path;
	char perm[5], dev[6], mapname[PATH_MAX], line_buf[BUFSIZ + 1];
	unsigned char *mem_buf;

	/*Handle Ctrl-c interrupt*/
	act.sa_handler = intHandler;
	sigaction(SIGINT, &act, NULL);

	/*Get all those args!*/
	l = "all";
	pid = -1;
	show = 0;
	while((ch = getopt(argc, argv, "p:l:sh")) != -1){
		switch(ch){
			case 'p':
				pid = atoi(optarg);
				break;
			case 'l':
				l = optarg;
				break;
			case 's':
				show = 1;
				break;
			case 'h':
				printf("Available options:\n");
				printf("\tp <pid> - PID of the process you want to dump\n");
				printf("\tl <list,of,plugins | all> - Load only certain plugins, comma separated\n");
				printf("\ts - Show available plugins\n");
				printf("\th - Show this help\n");
		}
	}

	/*We need a PID for sure...*/
	if(pid == -1 && !show){
		printf("Usage: %s -p <pid> -l <plugin list | all>\n", argv[0]);
		printf("-p argument is mandatory.\n");
		return EXIT_FAILURE;
	}

	if(show){
		printf("Available plugins:\n");
	}

	/*Vars used for operations on the linked list*/
	plugin_t *tmp, *curr, *head = NULL;

	/*Open the plugins folder...*/
	if((dir = opendir(PLUGINS_DIR)) == NULL){
		printf("Can't open plugins dir\n");
		return EXIT_FAILURE;
	}

	/*... and read it's content*/
	while((ent = readdir(dir)) != NULL){
		/*Make vars shorter*/
		fname = ent->d_name;
		fnlen = strlen(fname);

		/*If the file name is shorter than 3 chars or it doesn't end with '.so'*/
		/*then it's not a possible plugin, so just skip it*/
		if(fnlen <= 3 || strcmp(fname + fnlen - 3, ".so") != 0 ){
			continue;
		}

		printf("Loading %s... ", fname);

		/*Allocate some space for the path to the plugin, NULL-terminated*/
		lib_path = calloc(1, strlen(PLUGINS_DIR) + fnlen + 1);
		sprintf(lib_path, "%s%s", PLUGINS_DIR, fname);
		/*Load the plugin*/
		plugin_handle = dlopen(lib_path, RTLD_NOW);
		free(lib_path);

		/*Error trying to load the .so file we found. Maybe not a plugin?*/
		/*Keep going anyways...*/
		if(!plugin_handle){
			printf("Error loading plugin!\n%s\n", dlerror());
			continue;
		}

		/*Init the plugin by calling it's 'init' function*/
		plugin_t *(*initf)();
		*(plugin_t **) (&initf) = dlsym(plugin_handle, "init\0");

		if((error = dlerror()) != NULL){
			/*Oops... something wen't wrong calling the plugin's init function*/
			printf("Error loading init function!\n%s\n", error);
			continue;
		}

		printf("Ok\n");

		/*Everything went fine, lets create/update the linked list containing*/
		/*all the loaded plugins*/
		if(!head){
			curr = head = (*initf)();
		}else{
			curr->next = (*initf)();
			tmp = curr;
			curr = curr->next;
		}
		curr->hndl = plugin_handle;

		if(show){
			printf("\t%s - %s\n", curr->name, curr->description);
		}

		/*Check if we should actually load the plugin*/
		if(strcasestr(l, curr->name) == NULL && strcmp(l, "all") != 0){
			printf("\tPlugin does not match with load list, unloading %s...\n", curr->name);

			/*Close handle*/
			dlclose(curr->hndl);

			/*Free memory*/
			free(curr);

			/*Get back to the previous plugin*/
			curr = tmp;
		}
	}

	/*We are done, close the plugins folder*/
	closedir(dir);



	/*===*/



	/*Keep running until we get an interrupt*/
	while(keepRunning && !show){

		/*Attach to the process*/
		if(ptrace(PTRACE_ATTACH, pid, NULL, NULL) == -1){
			printf("Error attaching to process.\n");
			return EXIT_FAILURE;
		}
		waitpid(pid, NULL, 0);

		/*Open the fds from where we're going to read*/
		snprintf(path, sizeof(path), "/proc/%d/maps", pid);
		maps_fd = fopen(path, "r");
		snprintf(path, sizeof(path), "/proc/%d/mem", pid);
		mem_fd = open(path, O_RDONLY);

		if(!maps_fd || mem_fd == -1){
			printf("Error opening /proc/%d/maps or /proc/%d/mem\n", pid, pid);
			continue;
		}

		/*We're ready to read*/
		while(fgets(line_buf, BUFSIZ, maps_fd) != NULL){

			/*Quit faster if we receive Ctrl-c*/
			if(!keepRunning){
				break;
			}

			mapname[0] = '\0';
			sscanf(line_buf, "%llx-%llx %4s %lx %5s %ld %s", &start, &end, perm, &foo, dev, &inode, mapname);
			/*We don't want to read the memory of shared libraries*/
			if(access(mapname, R_OK) == 0){
				continue;
			}

			mem_size = end - start;
			mem_buf = malloc(mem_size);
			if(!mem_buf){
				printf("Error allocating space!\n");
				continue;
			}
			lseek64(mem_fd, start, SEEK_SET);
			if(read(mem_fd, mem_buf, mem_size) < mem_size){
				printf("Error copying raw memory! (%s)\n", strerror(errno));
				free(mem_buf);
				continue;
			}

			/*Give access to the memory block to each plugin*/
			curr = head;
			while(curr != NULL){
				void *(*f)(unsigned char *mem_buf, unsigned long int mem_size);
				*(void **) (&f) = dlsym(curr->hndl, "process\0");

				(*f)(mem_buf, mem_size);
				curr = curr->next;
			}

			free(mem_buf);
		}

		/*Dettach from the process*/
		ptrace(PTRACE_DETACH, pid, NULL, NULL);

		/*Close file descriptors*/
		mem_fd != -1 && close(mem_fd);
		maps_fd && fclose(maps_fd);
	}



	/*===*/



	/*We are about to quit*/
	/*Make sure to unload all the plugins we loaded...*/
	curr = head;
	while(curr != NULL){
		printf("Unloading %s...\n", curr->name);

		/*Close handle...*/
		dlclose(curr->hndl);

		/*go to the next plugin in the linked list...*/
		tmp = curr;
		curr = curr->next;

		/*and free all the memory we allocated at some point*/
		free(tmp);
	}

	/*Bye bye!*/
	return EXIT_SUCCESS;
}