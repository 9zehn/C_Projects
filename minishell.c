#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <time.h>
#include <string.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#define BRIGHTBLUE "\x1b[34;1m"
#define DEFAULT    "\x1b[0m"

volatile sig_atomic_t interrupted = false;

void catch_signal(int signal) {
	if (signal == SIGINT) {
		interrupted = true;
	}
}	

int main (){

	char cwd[PATH_MAX];
	char home_dir[PATH_MAX];
	home_dir[0] = 0;
	char buf[4096];
	char** args = malloc(2048 * sizeof(char*));
	bool free_r;
	int arg_index = 0;

	int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    flags |= O_NONBLOCK;
    fcntl(STDIN_FILENO, F_SETFL, flags);

	
	// Set up signal handler
	struct sigaction action;
	memset(&action,0,sizeof(struct sigaction));
	action.sa_handler = catch_signal;
    action.sa_flags = SA_RESTART;
   	if (sigaction(SIGINT, &action, NULL) == -1) {
       	perror("sigaction(SIGINT)");
       	return EXIT_FAILURE;
   	}

	// Set no buffering for stdin
	if (setvbuf(stdin,NULL,_IONBF,0)!=0){
		fprintf(stderr,"Error: Cannot set buffer mode. %s.\n",strerror(errno));
		return EXIT_FAILURE;
	}

	while(true){

		// Get current working directory
		if (getcwd(cwd,PATH_MAX-1) == NULL){
			fprintf(stderr,"Error: getcwd() failed. %s.\n",strerror(errno));
			return EXIT_FAILURE;
		}

		// Display current directory
		if (interrupted == true) { 
			interrupted = false;
			printf("\n[%s%s%s]$ ",BRIGHTBLUE,cwd,DEFAULT);
		}
		else{
			printf("[%s%s%s]$ ",BRIGHTBLUE,cwd,DEFAULT);
		}
		fflush(stdout);

		// Get user args updated
		char c;
		int counter = 0;
		bool crtld = false;
		buf[0] = '\0';
		
		while(true){
			c = getc(stdin);


			if (interrupted == true) {
				break;
			}

			if(counter == 0 && c == ' '){
				continue;
			}
			if(c == '\n'){
				break;
			}
			else if (c == EOF){
				if (errno == EAGAIN || errno == EWOULDBLOCK) {
					continue;
				}
			}
			else{
				buf[counter++] = c;
			}
		}

		if(interrupted){
			continue;
		}

		buf[counter] = '\0';

		if(crtld || buf[0] == '\0'){
			continue;
		}

		// Free some stuff
		if(arg_index > 0){
			for(int l = 0; l <= arg_index; l++){
				free(args[l]);
			}
			free_r = false;
		}

		// Input handler
		int count = 0, word_index = 0;// quot_count = 0;
		arg_index = 0;
		bool quotation = false;
		args[0] = malloc(4096 * sizeof(char));
		args[arg_index][0] = '\0';
		
		while (count < strlen(buf)){
			if(buf[count] == ' ' && quotation == false){
				args[arg_index++][word_index] = '\0';
				word_index = 0;
				args[arg_index] = malloc(4096*sizeof(char));
				args[arg_index][0] = '\0';
			}
			else if(buf[count] == '"'){
				if(buf[count+1]=='"'){
					count += 2;
					continue;
				}
				if(quotation){
					quotation = false;
					count++;
					continue;
				}
				quotation = true;			

			}
			else{
				args[arg_index][word_index++] = buf[count];
			}
			count++;
		}
		if (quotation){
			fprintf(stderr,"Error: Invalid expression.\n");
			continue;
		}

		arg_index++;
		//args[arg_index] = malloc(sizeof(char*));
		args[arg_index] = NULL;

		free_r = true;

		// Parse arguments, COMMAND: cd 
		if (strcmp(args[0],"cd") == 0){

			if(arg_index > 2){
				fprintf(stderr,"Error: too many arguments for cd.\n");
				continue;
			}

			// Input is cd | cd ~
			if (arg_index == 1 || (args[1][0] == 126 && strlen(args[1]) == 1 && arg_index == 2)
				|| (arg_index == 2 && args[1][0] == '\0')){ //126 is ~

				struct passwd* pwd = NULL;
				if ((pwd = getpwuid(getuid())) == NULL){
					fprintf(stderr,"Error: Cannot get password entry. %s.\n",strerror(errno));
					return EXIT_FAILURE;
				}
				if (sscanf(pwd->pw_dir,"%s",cwd) == 0 || sscanf(pwd->pw_dir,"%s",home_dir) == 0){
					fprintf(stderr,"Error: sscanf() failed. %s.\n",strerror(errno));
					return EXIT_FAILURE;
				}

				if (chdir(cwd) < 0){
					fprintf(stderr,"Error: Cannot change directory to '%s'. %s.\n",cwd,strerror(errno));
					continue;
				}

			} 

			// Input is ~/dir
			else if (args[1][0] == 126 && strlen(args[1]) > 1){
				if(home_dir[0] == 0){

					struct passwd* pwd2 = NULL;
					if ((pwd2 = getpwuid(getuid())) == NULL){
						fprintf(stderr,"Error: Cannot get password entry. %s.\n",strerror(errno));
						return EXIT_FAILURE;
					}
					if (sscanf(pwd2->pw_dir,"%s",home_dir) == 0){
						fprintf(stderr,"Error: sscanf() failed. %s.\n",strerror(errno));
						return EXIT_FAILURE;
					}
				}

				strcpy(cwd,home_dir);
				strcat(cwd,args[1]+1);

				if (chdir(cwd) < 0){
					fprintf(stderr,"Error: Cannot change directory to test '%s'. %s.\n",cwd,strerror(errno));
					continue;
				}

			}

			else{
				if(chdir(args[1]) < 0){
					fprintf(stderr,"Error: Cannot change directory to '%s'. %s.\n",args[1],strerror(errno));
					continue;
				}
				
			}
		}
		// TODO: Process other inputs 
		else if (strncmp(buf,"exit",4) == 0){
			if(arg_index > 0 && free_r){
				for(int l = 0; l <= arg_index; l++){
					free(args[l]);
				}
			}
			free(args);
			return EXIT_SUCCESS;

		}
		// Non-cd/non-exit inputs
		else {
			// program forks
			// 	pid shit, fork, and check for fork errors ("Error: fork() failed. %s.\n") 
			//
			// child will exec the input 
			// 	check for exec errors ("Error: exec() failed. %s.\n")
			//
			// parent will wait for child to finish
			// 	wait and wait errors ("Error: wait() failed. %s.\n")
			//
			pid_t pid;

			if ((pid = fork()) < 0) {
				fprintf(stderr, "Error: fork() failed. %s.\n", strerror(errno));
				return EXIT_FAILURE;
			} else if (pid == 0) {
				char *argv[4096];
				int count = 0;
				char *str = strtok(buf, " ");
				while (str != NULL) {
					argv[count++] = strdup(str);
					str = strtok(NULL, " ");
				}
				argv[count] = NULL;


				if ((execvp(argv[0], argv)) == -1) {
					fprintf(stderr, "Error: exec() failed. %s.\n", strerror(errno));
					return EXIT_FAILURE;
				}
			}
			pid_t w;
			if ((w = wait(NULL)) == -1) {
				fprintf(stderr, "Error: wait() failed. %s.\n", strerror(errno));
				return EXIT_FAILURE;
			}

		}

		fflush(stdout);

	}

	// FREE MALLOCS!!!!!

	return EXIT_SUCCESS;

}
