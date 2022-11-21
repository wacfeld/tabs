#include <stdio.h>
#include <assert.h>

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

// read lines from stream, write definitions into defs, return number of defs
int read_config(FILE *stream, struct def *defs)
{
  char s[MAX_LINE];
  int i = 0;
  
  while(fgets(s, MAX_LINE, stream))
  {
    if(!strcmp(s, "START\n")) // config over
      break;
    
    struct def d;
    
    // read fields from s into d
    char symb[2]; // intermediate storage is needed because %1s has to be used instead of %c, which does not skip whitespace
    char trail[2]; // for detecting trailing non-whitespace characters
    int res = sscanf(s, "%2s %1s %d %d %1s", d.lab, symb, &d.amt, &d.note, trail);
    d.symb = *symb;
    
    // check for comments and syntax errors
    if(res >= 1 && d.lab[0] == '#') // comment, skip
      continue;
    if(res == EOF) // whitespace-only line, skip
      continue;
    if(res < 4) // not whitespace-only, inadequate input
      fprintf(stderr, "read_config: missing fields\n%s", s);
    if(res == 5) // trailing non-whitespace characters
      fprintf(stderr, "read_config: trailing characters\n%s", s);

    // append d to defs
    defs[i++] = d;
  }

  return i;
}
