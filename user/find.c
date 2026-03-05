#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"


char*
fmtname(char *path) 
{
  static char buf[DIRSIZ+1] = { 0 };  /*static 关键问题：短文件名覆盖*/
  char *p;

  // Find first character after last slash.
  for(p=path+strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;

  // Return blank-padded name.
  if(strlen(p) >= DIRSIZ)
    return p;
  memmove(buf, p, strlen(p));
  buf[strlen(p)] = 0;
  return buf;
}


/*1.dir 2.filename*/
void findFile(char *path,char *filename){
	char buf[128], *p;
  	int fd;
	struct dirent de;
	struct stat st;
	if((fd = open(path, 0)) < 0){
		fprintf(2, "cannot open %s\n", path);
    		return;
  	}

  	if(fstat(fd, &st) < 0){
    		fprintf(2, "cannot stat %s\n", path);
    		close(fd);
    		return;
  	}

	switch(st.type){
		case T_DEVICE:
			break;
		case T_FILE:
			if(!strcmp(fmtname(path),filename)){
				printf("%s\n",path);
			}
			break;
		case T_DIR:
			if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
      				printf("ls: path too long\n");
      				break;
    			}
    			strcpy(buf, path);
    		        p = strlen(buf) + buf;
    		        *p++ = '/';  
    			while(read(fd, &de, sizeof(de)) == sizeof(de)){
      				if(de.inum == 0)
        				continue;
    				if(!strcmp(de.name, ".") || !strcmp(de.name, ".."))
    					continue; 
    				strcpy(p,de.name);
    				findFile(buf,filename);
    			}
    		break;
	}
	close(fd);
		
}


int main(int argc, char *argv[]){
	if(argc < 3){
		printf("argv empty\n");
		exit(-1);
	}
	char *path = argv[1];
	char *filename = argv[2];

	findFile(path,filename);
	exit(0);
}
