#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"


char **argvMaker(char *buf){
	static char *argv[MAXARG];
	int i = 1;
	char *p = buf;
	argv[i-1] = buf;
	while(*p != '\0'){
		if(*p == ' '){
			*p = '\0';
			if(p[1] != '\0'){
				argv[i] = p + 1;
				i++;
			}
		}
		p++;
	}
	argv[i] = 0;
	
	return argv;
}






int xargsFunc(int argc, char *oldArgv[]){
	char *newArgv[MAXARG] = { 0 };
	char buf[128];
	char c;
	int i = 0;
	int j = 0;
	
	while(read(0,&c,sizeof(char))){
		if(c != '\n')
			buf[i] = c;
		else{
			j++;
			buf[i] = '\0';
			i = 0;
			char **argv = argvMaker(buf);
			if(fork() == 0){
				memcpy(newArgv,oldArgv, argc * sizeof(char*));
				memcpy(newArgv + argc , argv, (MAXARG - argc) * sizeof(char*));
				exec(newArgv[0],newArgv);
			}		
		}
		i++;
	
	}
	
	if(j == 0){
		exec(oldArgv[0],oldArgv);
	}
	
	for(int z = 0; z < j; z++)
		wait(0);
		
	exit(0);
	
	return 0;
}


int main(int argc, char *argv[]){
	char *oldArgv[MAXARG] = { 0 };
	if(argc < 2){
                printf("arg empty");
                exit(-1);
        }else if(argc > MAXARG){
                printf("args too many");
                exit(-1);
        }

        memcpy(oldArgv,argv + 1, (argc - 1) * sizeof(char*));

	xargsFunc(argc - 1 , oldArgv);
	
	exit(0);
	
	return 0;

}

