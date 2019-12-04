#include "noam_utility.h"
#include "noam_lexer.h"

int noam_atob(const char* str){
    if(strcmp(str, NOAM_TRUE_STR) == 0){
        return 1;
    }
    if(strcmp(str, NOAM_FALSE_STR) == 0){
        return 0;
    }
    //TODO: Error
    return -1;
}