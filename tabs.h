#ifndef TABS_HEAD
#define TAB_HEAD

#define LABEL_LEN 2
#define MAX_DEFS 1000
#define MAX_LINE 100

#define TICKS_PER_QUARTER 256

#define REST '-'

struct def
{
  char lab[LABEL_LEN+1]; // label
  char symb;             // symbol
  int amt;               // amount
  int vol;               // volume
  int note;              // note
};

void read_tabs(FILE *in, FILE *out);

#endif
