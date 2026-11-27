#include "task.h"
#include <assert.h>
#include <stdio.h>
int main(void){for(int i=0;i<1000;i++){task_demo();assert(task_steps(0)==5 && task_steps(1)==5);}puts("PASS: 1000 cooperative scheduler runs on host");}
