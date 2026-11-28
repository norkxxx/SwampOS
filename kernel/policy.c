#include "policy.h"
#include "vm.h"
int policy_check_mapped(uint64_t grants,uint64_t syscall,uint64_t address,size_t length,int mapped){
 int data=mapped&&length&&length<=256&&address>=USER_DATA&&address<USER_DATA+4096&&length<=USER_DATA+4096-address;
 switch(syscall){
 case SWP_SYS_EXIT:case SWP_SYS_CAPS:case SWP_SYS_ID:case SWP_SYS_YIELD:case SWP_SYS_ROLE:return 0;
 case SWP_SYS_SEND:if(!(grants&CAP_IPC))return -1;return (data||user_readable(address,length))?0:-14;
 case SWP_SYS_RECV:if(!(grants&CAP_IPC))return -1;return (data||user_writable(address,length))?0:-14;
 case SWP_SYS_SCRATCH_WRITE:if(!(grants&CAP_SCRATCH))return -1;return (data||user_readable(address,length))?0:-14;
 case SWP_SYS_SCRATCH_READ:if(!(grants&CAP_SCRATCH))return -1;return (data||user_writable(address,length))?0:-14;
 case SWP_SYS_DATA_READ:if(!(grants&CAP_DATA))return -1;return (data||user_writable(address,length))?0:-14;
 case SWP_SYS_GUI_PUBLISH:if(!(grants&CAP_GUI))return -1;if(length!=GUI_FRAME_BYTES)return -22;return (data||user_readable(address,length))?0:-14;
 case SWP_SYS_WRITE:if(!(grants&CAP_CONSOLE))return -1;return (data||user_readable(address,length))?0:-14;
 default:return -38;
 }
}

int policy_check(uint64_t grants,uint64_t syscall,uint64_t address,size_t length){return policy_check_mapped(grants,syscall,address,length,0);}
