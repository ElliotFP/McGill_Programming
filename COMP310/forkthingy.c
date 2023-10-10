#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

int main(){
 	int x = 100;
	if (fork() == 0){
		x = 200;
		if (fork()==0){
			x=300;
		}
	}
	printf("x = %d\n", x);
	return 0;	
}

