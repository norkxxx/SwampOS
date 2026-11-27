#include "vm.h"
int user_readable(uint64_t a,size_t n){
 if(!n||n>256)return 0;
 return (a>=USER_CODE&&a<USER_CODE+4096&&n<=USER_CODE+4096-a)||
        (a>=USER_STACK&&a<USER_TOP&&n<=USER_TOP-a);
}
