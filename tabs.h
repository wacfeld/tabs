#ifndef TABS_HEAD
#define TAB_HEAD

#define LABEL_LEN 2
#define MAX_LINE 1000

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

// used to order notes within a bar
struct note
{
  int time; // time since start of line
  struct bytes on; // note on event without delta
  struct bytes off; // note off event without delta
};

void read_tabs(FILE *in, FILE *out);

#endif
