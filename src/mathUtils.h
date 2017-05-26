#ifndef MATHUTILS_H
#define MATHUTILS_H

//Not having just min() and max() seems uncanny, 
//but issue#398 of the framework reveals that they just don' work.
//Until there is a fix, this is what we got to do.
int min(int a, int b);
int max(int a, int b);
int saturate(int num, int min, int max);

#endif
