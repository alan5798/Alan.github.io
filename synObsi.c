#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
int main(int argc,char* argv[]){
	if(argc!=2){
		printf("Incorrect number of arguments\n");
		printf("print ./synchronous_Obsidian <the file or folder name you want to add>\n");
		return 0;
	}
	if(strcmp(argv[1],"-h")==0 || strcmp(argv[1],"--help")==0){
		printf("print ./synchronous_Obsidian <the file or folder name you want to add>\n");
	}
	if(strlen(argv[1])>50){
		printf("Name too long");
		return 0;
	}
	pid_t pid=fork();
	if(pid<0){
		printf("process alloc fail\n");
		return 0;
	}
	if(pid==0){//it is the child process
		char to_move[256]="/mnt/c/Users/18301/Documents/Obsidian Vault/";
		strcat(to_move,argv[1]);
		printf("%s\n",to_move);
		char* args[]={"cp","-r",to_move,"./source/_posts",NULL};
		if(execvp("cp",args)==-1){
			printf("command process fail\n");
		};
	}else{
		wait(NULL);
		printf("the move is done.");
		return 0;
	}
}

	

