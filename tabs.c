#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>

#include "tabs.h"

/* 
   all config lines are of the form
   <label> <symbol> <amount> <note>
   e.x.
   CC l 1 1001
   (for left crash)
   
   SN w 2 1002
   (for double stroke on snare)

   CC r 8 1001
   (roughly a cymbal roll)
*/

// global mutables, initialized in main()
struct def *defs;
int ndefs;

// process command passed by read_tabs()
void proc_command(char *s)
{
  assert(*s == '!'); // command start
  
  if(!strncmp(s+1, "def", 3) && isspace(s[4])) // definition
  {
    struct def d;
    
    // example line:
    // !def SN w 2 1001
    
    // read fields from s into d
    char symb[2]; // intermediate storage is needed because %1s has to be used instead of %c, which does not skip whitespace
    char trail[2]; // for detecting trailing non-whitespace characters
    int n = sscanf(s+4, "%2s %1s %d %d %1s", d.lab, symb, &d.amt, &d.note, trail);
    d.symb = *symb;
    
    // check for syntax errors
    if(n == EOF) // whitespace-only line, skip
      return;
    if(n < 4) // not whitespace-only, inadequate input
    {
      fprintf(stderr, "read_config: missing fields; skipping\n%s", s);
      return;
    }
    if(n == 5) // trailing non-whitespace characters
    {
      fprintf(stderr, "read_config: trailing characters; skipping\n%s", s);
      return;
    }

    // append d to defs
    defs[ndefs++] = d;
  }
}

// read tabs from stream, create midi output
void read_tabs(FILE *stream)
{
  char s[MAX_LINE];
  
  while(fgets(s, MAX_LINE, stream))
  {
    if(*s == '#') // comment, skip
      continue;
    
    if(*s == '!') // command, pass to proc_command()
    {
      proc_command(s);
      continue;
    }
  }
}
