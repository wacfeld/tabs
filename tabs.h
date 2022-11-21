#ifndef TABS_HEAD
#define TAB_HEAD

#define LABEL_LEN 2
#define MAX_DEFS 1000

struct def
{
  char lab[LABEL_LEN+1];
  char symb;
  int amt;
  int note;
};

void read_config(FILE *stream);

#endif
