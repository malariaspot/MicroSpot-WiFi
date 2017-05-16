#include "charUtils.h"

int getCharIndex(int from, char * buffer, const char * control){
  if(*control == '\0'){
    return from;
  }else{
    if(*(buffer + from) == '\0'){
      return -1;
    }else{
      if(*(buffer + from) == *control){
        int last = getCharIndex(from, buffer + 1, control + 1);
        if(last < 0){
          return getCharIndex(from + 1, buffer, control);
        }else{
          return last;
        }
      }else{
        return getCharIndex(from + 1, buffer, control);
      }
    }
  }
}

int getCharIndex(char * buffer, const char * control){
  return getCharIndex(0,buffer,control);
}
