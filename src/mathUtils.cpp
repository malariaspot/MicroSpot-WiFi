#include "mathUtils.h"

int min(int a, int b){
  return a < b ? a : b;
}

int max(int a, int b){
  return a > b ? a : b;
}

int saturate(int num, int min, int max){
  return num > max ? max :
         num < min ? min : num;
}
