#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "midi.h"
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
int maxdefs = 4;

// array of parts at any given time, initialized in read_tabs()
char (*parts)[MAX_LINE] = NULL;
int nparts = 0;
int maxparts = 4;

// variables
// these are hard-coded because there aren't many of them and they're all unique

// time signature
uint sig_numer;
uint sig_denom;

// tempo in bars per minute
uint bpm;

// subdivision of whole note (quarters, eights, etc.) that a single character in the tablature represents
// e.x. if subdiv == 4 then 'o' is a quarter note, 'w' (double) is an eighth note
uint subdiv;

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
  if(ndefs == maxdefs) // expand if necessary
  {
    maxdefs *= 2;
    defs = realloc(defs, sizeof(struct def) * maxdefs);
    assert(defs);
  }
  defs[ndefs++] = d;
}

void proc_set(char *s, FILE *out)
{
  char name[MAX_LINE]; // variable name
  char trail[2] = {0,0}; // for detecting trailing non-whitespace characters

  if(sscanf(s, "%s", name) == EOF)
    error("missing variable name\n");

  // go through possible variables case-by-case
  if(!strcmp(name, "sig")) // time signature
  {
    // read numerator and denominator
    if(sscanf(s, "%*s %u %u %1s", &sig_numer, &sig_denom, trail) != 2)
      error("sig: syntax error\n");

    // check validity of time signature
    {
      if(!sig_numer)
        error("sig: numerator 0\n");
      if(!sig_denom)
        error("sig: denominator 0\n");
      
      uint temp = sig_denom;
      while(temp != 1)
      {
        if(temp % 2 != 0)
          error("sig: denominator not power of 2\n");
        temp /= 2;
      }
    }

    // output time signature change
    struct bytes ev = make_timesig(sig_numer, sig_denom); // create event
    struct bytes mtrk_ev = make_mtrk_event(0, ev, 1); // prepend delta
    put_bytes(out, mtrk_ev, 1); // output
  }

  if(!strcmp(name, "bpm")) // tempo
  {
    if(scanf(s, "%*s %u %1s", &bpm, trail) != 1)
      error("bpm: syntax error\n");

    // check tempo is valid
    if(bpm == 0)
      error("bpm: bpm is 0\n");

    // output tempo change
    struct bytes ev = make_tempo(bpm, sig_numer, sig_denom); // create event
    struct bytes mtrk_ev = make_mtrk_event(0, ev, 1); // prepend delta
    put_bytes(out, mtrk_ev, 1); // output
  }

  if(!strcmp(name, "div")) // subdivision
  {
    if(scanf(s, "%*s %u %1s", &subdiv, trail) != 1)
      error("div: syntax error\n");

    // check subdivision is valid
    {
      if(!subdiv)
        error("div: subdivision is 0\n");
      
      uint temp = subdiv;
      while(temp != 1)
      {
        if(temp % 2 != 0)
          error("div: subdivision not power of 2\n");
        temp /= 2;
      }
    }
  }

  // no such variable, print error
  error("variable \"%s\" does not exist\n", name);
}

// process command passed by read_tabs()
void proc_command(char *s, FILE *out)
{
  assert(*s == '!'); // command start
  
  if(!strncmp(s+1, "def", 3) && isspace(s[4])) // definition
  {
    proc_def(s+4); // write into defs
  }
  
  if(!strncmp(s+1, "set", 3) && isspace(s[4])) // variable assignment
  {
    proc_set(s+4, out); // write into varnames, varvals; 
  }
}

// return true if line s is all whitespace
int blankline(char *s)
{
  int blank = 1;
  
  for(int i = 0; i < strlen(s); i++)
  {
    if(!isspace(s[0]))
      blank = 0;
  }

  return blank;
}

// process parts[0] (could be command comment, blank, or tablature)
// if tablature, read the following lines into parts[] as well
void proc_line(FILE *in, FILE *out)
{
  char *s = parts[0];
  
  if(s[strlen(s)-1] != '\n') // full line was not read
    error("maximum line length (%d) exceeded\n", MAX_LINE);

  if(blankline(s)) // blank line, skip
    return;

  if(*s == '#') // comment, skip
    return;

  if(*s == '!') // command, pass to proc_command()
  {
    proc_command(s, out);
    return;
  }

  // if none of the above, it's the start of a block of tablature

  // read parts until blank line
  nparts = 0;
  
}

// read tabs from in, write midi output to out
void read_tabs(FILE *in, FILE *out)
{
  // put header
  // format 0, 1 track, _ ticks per quarter note
  put_bytes(out, make_header(0, 1, TICKS_PER_QUARTER), 1);
  
  // struct bytes sig = make_timesig(4,4); // default time signature
  // struct bytes tempo = make_tempo(
  
  // allocate defs, parts
  defs = malloc(sizeof(struct def) * maxdefs);
  parts = malloc(sizeof(char[MAX_LINE]) * maxparts);
  assert(defs);
  assert(parts);

  // process lines
  while(fgets(parts[0], MAX_LINE, in)) // read into parts[0], so that if it's the start of a block we don't have to strcpy it over
  {
    linenum++;
    proc_line(in, out);
  }

  free(defs);
  defs = 0;
  free(parts);
  parts = 0;
}
