#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include "ancestor.h"

#define __NR_cs3013_syscall2 378

int main () {
  // Testing part 1
  char str[5];
  int fd = open("butt.txt",0);
  size_t bytes = read(fd,str,5);
  close(fd);
  printf("%s\n",str);
  // Testing part 2
  struct ancestry grandpa;
  int status, status1;

	printf("The return values of the system call is:\n");
	unsigned short cpid, ppid, cpid1;
	cpid = getpid() +1; //child
	ppid = getpid(); //parent
  cpid1 = getpid() + 2; //second child
	int rc1 = fork();
  int rc = fork();
	if(rc < 0 || rc1 < 0) {
    exit(1);
    return -1;
  }
	else if(rc == 0){
		long jj = (long) syscall(__NR_cs3013_syscall2, &ppid, &grandpa);//call syscall
    printf("%ld\n",jj);
    exit(0);
	}else if (rc1 == 0){
    sleep(1);
    exit(0);
  }
	else{
		int rc_wait = waitpid(cpid, &status, 0);
    int rc_wait1 = waitpid(cpid1, &status1, 0);
	}
  printf("%d\n", (int)grandpa.children[0]);
  for (int i = 0;i <10 ;i++)printf("%d\n", grandpa.ancestors[i]);
	return 0;
}
