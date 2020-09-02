#include "Timer.h"

#include <stdlib.h>
#include <pthread.h>

int start(timer *t){
    if(!(pthread_create(/*TID*/, NULL, /*FUNCTION*/, /*args*/)==0)){
        return ERROR_PTHREAD_CREATE;
    }
    return 0;
}
