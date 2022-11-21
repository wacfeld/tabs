#ifndef TABS_HEAD
#define TAB_HEAD

#define LABEL_LEN 2
#define MAX_DEFS 1000
#define MAX_LINE 100

struct def
{
  char lab[LABEL_LEN+1]; // label
  char symb;             // symbol
  int amt;               // amount
  int vol;               // volume
  int note;              // note
};

void proc_command(char *s);
void read_tabs(FILE *stream);

#endif
