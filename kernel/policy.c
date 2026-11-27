#include "policy.h"
#include "vm.h"
int policy_check(uint64_t grants,uint64_t syscall,uint64_t address,size_t length){
 switch(syscall){
 case SWP_SYS_EXIT:case SWP_SYS_CAPS:return 0;
 case SWP_SYS_WRITE:if(!(grants&CAP_CONSOLE))return -1;return user_readable(address,length)?0:-14;
 default:return -38;
 }
}
