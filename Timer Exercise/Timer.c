#include "Timer.h"

#include <stdlib.h>
#include <pthread.h>
#include <time.h>

int start(timer *t){
    if(!(pthread_create(/*TID*/, NULL, /*FUNCTION*/, /*args*/)==0)){
        return ERROR_PTHREAD_CREATE;
    }
    return 0;
}

int startat(timer *t, int year, int month, int day, int h, int min, int sec){
    
    struct tm runtime;
    runtime.tm_year = year - 1900;
    runtime.tm_mon = month - 1;
    runtime.tm_mday = day;
    runtime.tm_hour = h;
    runtime.tm_min = min;
    runtime.tm_sec = sec;
    runtime.tm_isdst = -1; //get DST info from system


    long int extra_delay = difftime(time(NULL),mktime(&runtime));
    
    if( extradelay > 0 ){
        t->delay += extradelay;
    }
    
    return start(t);
}
