#ifndef LINE_H
#define LINE_H

struct Line{
  Line *prev;
  String content;
  Line *next;
};

#endif //LINE_H