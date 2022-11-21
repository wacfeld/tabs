#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "tabs.h"

// silly hack to make LABEL_LEN work (otherwise it would have to be hardcoded)
// https://stackoverflow.com/questions/70815044/how-to-make-macro-replacement-text-into-a-string-in-c-c
#define STRING(x) #x
#define ASTRING(x) STRING(x)

#define error(fmt, ...) do { fprintf(stderr, "%s: %s: line %d: " fmt, __FILE__, __func__, linenum __VA_OPT__(,) __VA_ARGS__); exit(1); } while(0)

/* 
   all config lines are of the form
   <label> <symbol> <amount> <volume> <note>
   e.x.
   CC l 1 100 49
   (hit left crash)
   
   SN w 2 100 38
   (double stroke on snare)

   CC r 8 100 51
   (roughly a cymbal roll)

   SN g 1 50 38
   (ghost note on snare)
*/

// array of definitions, initialized in read_tabs()
struct def *defs;
int ndefs = 0;

// variables
// these are hard-coded because there aren't many of them and they're all unique

// time signature
int sig_numer;
int sig_denom;

// tempo in bars per minute
int bpm;

// error messages need to know line number
int linenum = 0;

// process definition
// example:
// SN w 2 100 1001
void proc_def(char *s)
{
  struct def d;

  // read fields from s into d
  char symb[2]; // intermediate storage is needed because %1s has to be used instead of %c, which does not skip whitespace
  char trail[2]; // for detecting trailing non-whitespace characters
  int n = sscanf(s, "%" ASTRING(LABEL_LEN) "s %1s %d %d %d %1s", d.lab, symb, &d.amt, &d.vol, &d.note, trail);
  d.symb = *symb;

  // check for syntax errors
  if(n < 4) // missing fields
  {
    error("missing fields\n");
  }
  if(n == 5) // trailing non-whitespace
  {
    error("trailing characters\n");
  }

  // append d to defs
  assert(defs);
  if(ndefs == MAX_DEFS)
  {
    error("maximum allowed definitions (%d) exceeded\n", MAX_DEFS);
  }
  defs[ndefs++] = d;
}

void proc_set(char *s)
{
  char name[MAX_LINE]; // variable name
  char trail[2] = {0,0}; // for detecting trailing non-whitespace characters

  if(sscanf(s, "%s", name) == EOF)
    error("missing variable name\n");

  // go through possible variables case-by-case
  if(!strcmp(name, "sig")) // time signature
  {
    if(sscanf(s, "%*s %d %d %1s", &sig_numer, &sig_denom, trail) != 2)
      error("sig: syntax error\n");
  }

  if(!strcmp(name, "bpm")) // tempo
  {
    if(scanf(s, "%*s %d %1s", &bpm, trail) != 1)
      error("bpm: syntax error\n");
  }

  // did not find match, print error
  error("variable \"%s\" does not exist\n", name);
}

// process command passed by read_tabs()
void proc_command(char *s)
{
  assert(*s == '!'); // command start
  
  if(!strncmp(s+1, "def", 3) && isspace(s[4])) // definition
  {
    proc_def(s+4); // write into defs
  }
  
  if(!strncmp(s+1, "set", 3) && isspace(s[4])) // variable assignment
  {
    proc_set(s+4); // write into varnames, varvals; 
  }
}

// read tabs from in, write midi output to out
void read_tabs(FILE *in, FILE *out)
{
  // put header
  // format 0, 1 track, _ ticks per quarter note
  put_bytes(out, make_header(0, 1, TICKS_PER_QUARTER));
  
  // struct bytes sig = make_timesig(4,4); // default time signature
  // struct bytes tempo = make_tempo(
  
  // initialize defs
  {
    struct def temp[MAX_DEFS];
    defs = temp;
  }
  
  char s[MAX_LINE];
  
  while(fgets(s, MAX_LINE, in))
  {
    linenum++;
    
    if(s[strlen(s)-1] != '\n') // full line was not read
    {
      error("maximum line length (%d) exceeded\n", MAX_LINE);
    }
    
    if(*s == '#') // comment, skip
      continue;
    
    if(*s == '!') // command, pass to proc_command()
    {
      proc_command(s);
      continue;
    }
  }

  // uninitialize defs
  defs = 0;
}
