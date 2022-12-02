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

#define append(arr, size, max, item) do{ if(size == max) { max++; max *= 2; arr = realloc(arr, max*sizeof(*arr)); } arr[size++] = item; } while(0)

#define dbg(fmt, ...) do {fprintf(stderr, fmt __VA_OPT__(,) __VA_ARGS__);}while(0)
#define putd(x) fprintf(stderr, #x ": %d\n", x)
#define error(fmt, ...) do { fprintf(stderr, "%s: %d: %s: line %d: " fmt, __FILE__, __LINE__, __func__, linenum __VA_OPT__(,) __VA_ARGS__); exit(1); } while(0)

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

struct track main_tr = {0, 0, NULL};

struct note *notes = NULL;
int nnotes = 0;
int mnotes = 0;

// variables
// these are hard-coded because there aren't many of them and they're all unique

// time signature
uint sig_numer = 0;
uint sig_denom = 0;

// tempo in bars per minute
uint bpm = 0;

// subdivision of whole note (quarters, eights, etc.) that a single character in the tablature represents
// e.x. if subdiv == 4 then 'o' is a quarter note, 'w' (double) is an eighth note
uint subdiv = 0;

// error messages need to know line number
int linenum = 1;
int time = 0; // time in ticks since start of piece, used to calculate deltas
int inbar = 0; // whether we're currently processing a bar or not

// append bytes b to track tr
void track_app(struct track *tr, struct bytes b)
{
  if(tr->n == tr->max)
  {
    tr->max++;
    tr->max *= 2;
    tr->evs = realloc(tr->evs, sizeof(struct bytes) * tr->max);
  }
  
  tr->evs[tr->n++] = b;
}

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
  
  // if(n == 5) // trailing non-whitespace
  // {
  //   error("trailing characters %d\n", *trail);
  // }

  // append d to defs
  assert(defs);
  if(ndefs == maxdefs) // expand if necessary
  {
    maxdefs *= 2;
    defs = realloc(defs, sizeof(struct def) * maxdefs);
    assert(defs);
  }
  defs[ndefs++] = d;

  dbg("added definition label %s symbol %c amount %d volume %d note %d\n", d.lab, d.symb, d.amt, d.vol, d.note);
}

void proc_set(char *s)
{
  char name[MAX_LINE]; // variable name
  char trail[2] = {0,0}; // for detecting trailing non-whitespace characters

  if(sscanf(s, "%s", name) == EOF)
    error("missing variable name\n");

  // go through possible variables case-by-case
  else if(!strcmp(name, "sig")) // time signature
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
    struct bytes mtrk_ev = make_mtrk_event(0, ev); // prepend delta
    track_app(&main_tr, mtrk_ev); // output

    dbg("set time signature to %d %d\n", sig_numer, sig_denom);
  }

  else if(!strcmp(name, "bpm")) // tempo
  {
    // if(scanf(s, "%*s %u %1s", &bpm, trail) != 1)
    if(sscanf(s, "%*s %u %1s", &bpm, trail) < 1)
      error("bpm: syntax error\n");

    // check tempo is valid
    if(bpm == 0)
      error("bpm: bpm is 0\n");

    // output tempo change
    // putd(sig_numer);
    // putd(sig_denom);
    struct bytes ev = make_tempo(bpm, sig_numer, sig_denom); // create event
    struct bytes mtrk_ev = make_mtrk_event(0, ev); // prepend delta
    track_app(&main_tr, mtrk_ev); // output

    dbg("set bpm to %d\n", bpm);
  }

  else if(!strcmp(name, "div")) // subdivision
  {
    // if(scanf(s, "%*s %u %1s", &subdiv, trail) != 1)
    if(sscanf(s, " %*s %u %1s", &subdiv, trail) < 1)
    {
      error("div: syntax error\n");
    }

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

    dbg("set subdiv to %d\n", subdiv);
  }

  // no such variable, print error
  else
    error("variable \"%s\" does not exist\n", name);
}

// process command passed by read_tabs()
void proc_command(char *s)
{
  assert(*s == '!'); // command start
  
  if(!strncmp(s+1, "def", 3) && isspace(s[4])) // definition
  {
    dbg("reading def...\n");
    proc_def(s+4); // write into defs
  }
  
  if(!strncmp(s+1, "set", 3) && isspace(s[4])) // variable assignment
  {
    dbg("reading set...\n");
    proc_set(s+4); // write into varnames, varvals; 
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

// read and process one line (could be command comment, blank, or tablature)
// if tablature, read the following lines into parts[] as well
// return 1 if tablature, -1 on EOF, 0 otherwise
int proc_line(FILE *in, char *s)
{
  if(!fgets(s, MAX_LINE, in)) // read 1 line
    return -1; // return -1 on EOF
  
  if(s[strlen(s)-1] != '\n') // full line was not read
    error("maximum line length (%d) exceeded\n", MAX_LINE);

  if(blankline(s)) // blank line, skip
    return 0;

  if(*s == '#') // comment, skip
    return 0;

  if(*s == '!') // command, pass to proc_command()
  {
    proc_command(s);
    return 0;
  }

  // if none of the above, it's a part; return 1 and let caller process s
  return 1;
}

// process one line of tab, write into notes
void proc_tab(char *s)
{
  dbg("tab in\n");
  // grab label
  char lab[3] = {s[0], s[1], 0};
  
  assert(strlen(s) >= LABEL_LEN);
  int time = 0; // time in ticks since start of line

  if(!subdiv)
  {
    dbg("subdiv hasn't been set; defaulting to 4\n");
    subdiv = 4;
  }
  int tps = TICKS_PER_QUARTER * 4 / subdiv; // calculate ticks per subdivision at the moment

  for(char *p = s + LABEL_LEN; *p; p++)
  {
    if(isspace(*p))
      continue;
    
    if(*p == REST)
    {
      time += tps;
      continue;
    }

    struct def *d = NULL;
    // lookup def
    for(int i = 0; i < ndefs; i++)
    {
      if(!strcmp(defs[i].lab, lab)
          && defs[i].symb == *p)
      {
        d = defs+i;
        // dbg("found def %d which has velocity %d\n", i, d->vol);
      }
    }

    if(!d)
      error("could not find definition for %s %c\n", lab, *p);
    
    // create note(s)
    int temp = time; // to stop rounding errors from adding up (e.x. from triplets), we use a separate variable to time sub-subdivisions
    for(int i = 0; i < d->amt; i++)
    {
      temp = time + (tps*i) / d->amt;
      putd(temp);
      // on and off midi events
      struct bytes on = make_midi_event(NOTE_ON, DRUM_CHANNEL, 2, d->note, d->vol);
      struct bytes off = make_midi_event(NOTE_OFF, DRUM_CHANNEL, 2, d->note, d->vol);
      dbg("velocity %d\n", d->vol);

      // combine into single note
      struct note non = {temp, on};

      int offtime = time + (tps * (i+1)) / d->amt;
      struct note noff = {offtime, off};
      
      // append to notes
      dbg("appending a note\n");
      append(notes, nnotes, mnotes, non);
      append(notes, nnotes, mnotes, noff);
    }

    time += tps;
  }
  dbg("tab out\n");
}

// compare two struct notes based on their times
int notecmp(const struct note *a, const struct note *b)
{
  return a->time - b->time;
}

// sort, write notes to main_tr, deallocate
void flush_notes()
{
  if(!nnotes)
    return;
  
  // sort all the notes by time
  qsort(notes, nnotes, sizeof(struct note), (int (*)(const void *, const void *)) notecmp);
  
  // write them all into main_tr with prepended deltas
  int prevtime = 0;
  for(int i = 0; i < nnotes; i++)
  {
    int del = notes[i].time - prevtime;
    prevtime = notes[i].time;
    
    struct bytes mev = make_mtrk_event(del, notes[i].ev);
    track_app(&main_tr, mev);
    // putd(main_tr.n);
  }
  
  free(notes);
  notes = NULL;
  nnotes = 0;
  mnotes = 0;
}

// read tabs from in, write midi output to out
void read_tabs(FILE *in, FILE *out)
{
  // put header
  // format 0; 1 track; _ ticks per quarter note
  put_bytes(out, make_header(0, 1, TICKS_PER_QUARTER));
  
  // struct bytes sig = make_timesig(4,4); // default time signature
  // struct bytes tempo = make_tempo(

  // TODO initialize track, append to track instead of call put_bytes()
  main_tr.evs = malloc(sizeof(struct bytes) * main_tr.max);
  
  // allocate defs
  defs = malloc(sizeof(struct def) * maxdefs);
  assert(defs);
  
  proc_set(" sig 4 4\n");
  proc_set(" bpm 15\n");

  // process lines
  int status;
  char s[MAX_LINE];
  while((status = proc_line(in, s)) != -1) // process lines until EOF
  {
    if(status == 1) // tablature
    {
      if(!inbar) // start of bar; initialize
      {
        inbar = 1;

        notes = NULL;
        nnotes = 0;
        mnotes = 0;
      }

      // pass to proc_tab()
      proc_tab(s);
    }
    
    else // status == 0
    {
      if(inbar) // end of bar; process stuff and then deallocate
      {
        inbar = 0;
        flush_notes();
      }
    }
    
    linenum++;
  }

  flush_notes();

  // TODO output end of track, calculate track length, etc.
  
  // end of track
  struct bytes end = make_bytes(3, 0xff, 0x2f, 0x00);
  track_app(&main_tr, make_mtrk_event(0,end));
  
  // create track chunk
  struct bytes chunk = make_track_chunk(main_tr);
  // output
  put_bytes(out, chunk);

  free(defs);
  defs = 0;
}
