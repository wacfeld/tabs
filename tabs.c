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

#define error(fmt, ...) do { fprintf(stderr, "%s: %s: line %d: " fmt, __FILE__, __func__, linenum __VA_OPT__(,) __VA_ARGS__); exit(1); } while(0);

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

// array of definitions, initialized in read_tabs()
struct def *defs;
int ndefs = 0;

// array of variables
char *varnames[] = {"bpm", "sig"};
#define NVARS (sizeof(varnames) / sizeof(*varnames))
int varvals[NVARS];


// error messages need to know line number
int linenum = 0;

// process definition
// example:
// SN w 2 1001
void proc_def(char *s)
{
  struct def d;

  // read fields from s into d
  char symb[2]; // intermediate storage is needed because %1s has to be used instead of %c, which does not skip whitespace
  char trail[2]; // for detecting trailing non-whitespace characters
  int n = sscanf(s, "%" ASTRING(LABEL_LEN) "s %1s %d %d %1s", d.lab, symb, &d.amt, &d.note, trail);
  d.symb = *symb;

  // check for syntax errors
  if(n < 4) // missing fields
  {
    error("missing fields\n");
    exit(1);
  }
  if(n == 5) // trailing non-whitespace
  {
    error("trailing characters\n");
    exit(1);
  }

  // append d to defs
  assert(defs);
  if(ndefs == MAX_DEFS)
  {
    error("maximum allowed definitions (%d) exceeded\n", MAX_DEFS);
    exit(1);
  }
  defs[ndefs++] = d;
}

void proc_set(char *s)
{
  // variable name and value
  char name[MAX_LINE];
  int val = 0;
  
  char trail[2]; // for detecting trailing non-whitespace characters
  
  int n = sscanf(s, "%s %d %1s", name, &val, trail); // read name and value
  
  // check for syntax errors
  if(n < 2) // missing fields
  {
    error("missing fields\n");
    exit(1);
  }
  if(n == 3) // trailing non-whitespace
  {
    error("trailing characters\n");
    exit(1);
  }

  // check if variable name exists
  for(int i = 0; i < NVARS; i++)
  {
    if(!strcmp(varnames[i], name)) // found a match
    {
      // set value
      varvals[i] = val;
      return;
    }
  }

  // did not find match, print error
  error("variable does not exist\n");
  exit(1);
}

// process command passed by read_tabs()
void proc_command(char *s)
{
  assert(*s == '!'); // command start
  
  if(!strncmp(s+1, "def", 3) && isspace(s[4])) // definition
  {
    proc_def(s+4);
  }
  
  if(!strncmp(s+1, "set", 3) && isspace(s[4])) // variable assignment
  {
    proc_set(s+4);
  }
}

// read tabs from stream, create midi output
void read_tabs(FILE *stream)
{
  // initialize defs
  {
    struct def temp[MAX_DEFS];
    defs = temp;
  }
  
  char s[MAX_LINE];
  
  while(fgets(s, MAX_LINE, stream))
  {
    linenum++;
    
    if(s[strlen(s)-1] != '\n') // full line was not read
    {
      error("maximum line length (%d) exceeded\n", MAX_LINE);
      exit(1);
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
