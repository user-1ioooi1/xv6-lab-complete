#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void pipeline(int p[2]){
	int buf[1];
	int childp[2];
	close(p[1]);
	pipe(childp);

	if(read(p[0],buf,4) <= 0){
		return;
	}
	if(fork()){
		printf("prime %d\n",buf[0]);
		int parentBuf[1];
		while(read(p[0],parentBuf,4)){
			if(parentBuf[0] % buf[0] != 0){
				write(childp[1],parentBuf,4);
			}
		}
		close(p[0]);
		close(childp[0]);
		close(childp[1]);
		wait(0);

	}else{
		pipeline(childp);

	}

}



int main(int argc, char *argv[]){
	int p[2];
	pipe(p);
	
	
	for(int i = 2; i <= 35; i++){
		write(p[1],&i,4);
	}

	pipeline(p);
	
	exit(0);

	return 0;
}
