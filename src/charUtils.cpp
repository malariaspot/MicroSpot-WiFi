#include "charUtils.h"

int getCharIndex(int from, char * buffer, const char * control){
  int index = 0;
  int ctrIndex = 0;
  int ctrLen = 0;
  while(control[ctrIndex] != '\0'){
    if(buffer[from + index] == '\0') return -1;
    if(control[ctrIndex] != buffer[from + index]){
      index -= ctrIndex;
      ctrIndex = 0;
    }else{
      ctrIndex++;
    }
    index++;
  }
  return ctrIndex == 0 ? -1 : from + index - ctrIndex;
}

int getCharIndex(char * buffer, const char * control){
  return getCharIndex(0,buffer,control);
}
