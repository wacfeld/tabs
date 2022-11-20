#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "midi.h"

void put_bytes(struct bytes b, int discard)
{
  for(int i = 0; i < b.len; i++)
  {
    putchar(b.b[i]);
  }

  if(discard) // no longer needed, call free()
    free(b.b);
}

void write_bytes(char *s, int n, ...)
{
  va_list ap;
  va_start(ap, n);

  for(int i = 0; i < n; i++)
  {
    s[i] = va_arg(ap, uchar);
  }
  
  va_end(ap);
}

struct bytes make_header(uint format, uint tracks, uint division)
{
  assert(format <= 0xFFFF);
  assert(tracks <= 0xFFFF);
  assert(division <= 0x7FFF); // bit 15 must be clear
  
  uchar *head = malloc(14);

  head[0] = 'M';
  head[1] = 'T';
  head[2] = 'h';
  head[3] = 'd';
  head[4] = 0;
  head[5] = 0;
  head[6] = 0;
  head[7] = 6;
  
  head[8] = format >> 8;
  head[9] = format % 0xFF;
  head[10] = tracks >> 8;
  head[11] = tracks % 0xFF;
  head[12] = division >> 8;
  head[13] = division % 0xFF;

  struct bytes b = {14, head};
  return b;
}

// create variable length quantity from integer
struct bytes make_vlq(uint n)
{
  uchar *vlq = malloc(4); // vlqs are at most 4 bytes
  
  // largest division allowed is 7 Fs
  assert(n <= 0x0FFFFFFF);
  
  // write reverse order into vlq
  int len = 0;
  while(n)
  {
    vlq[len] = n & 0x7F; // mask out lowest 7 bits
    vlq[len] |= 0x80;    // set bit 7
    
    len++;
    n >>= 7; // shift right 7 bits
  }

  vlq[0] &= 0x7F; // clear bit 7 of lowest significance byte
  
  // reverse bytes
  for(int i = 0; i < len/2; i++)
  {
    uchar temp = vlq[i];
    vlq[i] = vlq[len-1 - i];
    vlq[len-1 - i] = temp;
  }

  struct bytes b = {len, vlq};
  return b;
}

// concatenate y onto x, return x
struct bytes byte_cat(struct bytes x, struct bytes y)
{
  x.b = realloc(x.b, x.len + y.len); // make room for y.b
  memcpy(x.b + x.len, y.b, y.len); // append y.b to x.b
  
  x.len += y.len; // update length
  free(y.b); // discard copied contents
  
  return x;
}

struct bytes make_track_chunk(track tr)
{
  // calculate number of bytes in body
  uint body_len = 0;
  for(int i = 0; i < tr.n_evs; i++)
  {
    body_len += tr.evs[i].len;
  }

  // // write chunk type and 
  // uchar *chunk = malloc(
}

// create an MTrk event, consisting of a delta followed by an event
struct bytes make_mtrk_event(uint delta, struct bytes ev)
{
  struct bytes vlq = make_vlq(delta);       // convert delta into variable length quantity
  struct bytes b = byte_cat(vlq, ev);

  return vlq;
}

// create a time signature event given a numerator and denominator (e.x. cut time 2, 4)
struct bytes make_timesig(uchar numer, uchar denom)
{
  assert(numer);
  assert(denom);
  
  // take the log base 2 of denom
  uchar denom_lg = 0;
  while(denom != 1)
  {
    denom_lg++;
    assert(denom % 2 == 0); // denom must be a power of 2
    denom /= 2;
  }

  // time sigs event is 7 bytes
  uchar *sig = malloc(7);
  sig[0] = 0xFF;
  sig[1] = 0x58;
  sig[2] = 0x04;
  sig[3] = numer;
  sig[4] = denom_lg;
  sig[5] = 24; // clocks per metronome click
  sig[6] = 8; // 32nds per 24 clocks
  
  struct bytes b = {7, sig};
  return b;
}

// create a tempo event given beats per minute
// time signature is needed to calculate tempo
// 1 beat = 1 bar, for simplicity. 60 bpm is 60 bars per minute
struct bytes make_tempo(uint bpm, uchar numer, uchar denom)
{
  assert(numer);
  assert(denom);
  assert(bpm);
  
  uchar *tempo = malloc(6); // tempo event is 6 bytes
  tempo[0] = 0xFF;
  tempo[1] = 0x51;
  tempo[2] = 0x03;

  // calculate microseconds per quarter note:
  // quarters/bar = 4 * numer / denom
  // quarters/min = 4 * bpm * numer / denom
  // quarters/sec = 4 * bpm * numer / (60 * denom)
  // microseconds/quarter = 60 * 1000000 * denom / (4 * bpm * numer)
  long m = ((long) 60 * 1000000 * denom) / (4 * bpm * numer);

  // must fit in 24 bits
  assert(m <= 0xFFFFFF);

  // write into tempo[], most significant byte first
  tempo[5] = m % 0xFF;
  m >>= 8;
  tempo[4] = m % 0xFF;
  m >>= 8;
  tempo[3] = m % 0xFF;

  struct bytes b = {6, tempo};
  return b;
}
