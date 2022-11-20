#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

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

void write_bytes(uchar *s, int n, ...)
{
  va_list ap;
  va_start(ap, n);

  for(int i = 0; i < n; i++)
  {
    s[i] = va_arg(ap, int); // has to be int because unnamed args get promoted
  }
  
  va_end(ap);
}

struct bytes make_header(uint format, uint tracks, uint division)
{
  assert(format <= 0xFFFF);
  assert(tracks <= 0xFFFF);
  assert(division <= 0x7FFF); // bit 15 must be clear
  
  uchar *head = malloc(14);
  write_bytes(head, 8, 'M', 'T', 'h', 'd', 0, 0, 0, 6);
  
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
struct bytes byte_cat(struct bytes x, struct bytes y, int reall, int discard)
{
  if(reall) // for efficiency, some callers might have allocated beforehand
    x.b = realloc(x.b, x.len + y.len); // make room for y.b
  
  memcpy(x.b + x.len, y.b, y.len); // append y.b to x.b
  
  x.len += y.len; // update length

  if(discard) // discard copied contents, if instructed to
    free(y.b);
  
  return x;
}

struct bytes make_track_chunk(struct track tr)
{
  // calculate number of bytes in body
  unsigned long body_len = 0;
  for(int i = 0; i < tr.n_evs; i++)
  {
    body_len += tr.evs[i].len;
  }

  assert(body_len <= 0xFFFFFFFF); // must fit in 4 bytes
  
  // write chunk type and length
  uchar *chunk = malloc(8 + body_len);
  write_bytes(chunk, 4, 'M', 'T', 'r', 'k');
  chunk[4] = (body_len >> 24) % 0xFF;
  chunk[5] = (body_len >> 16) % 0xFF;
  chunk[6] = (body_len >> 8)  % 0xFF;
  chunk[7] = (body_len)       % 0xFF;
  
  struct bytes b = {8, chunk};
  
  // append tracks
  for(int i = 0; i < tr.n_evs; i++)
  {
    b = byte_cat(b, tr.evs[i], 0, 1); // do not realloc(), do free()
  }

  return b;
}

// create an MTrk event, consisting of a delta followed by an event
struct bytes make_mtrk_event(uint delta, struct bytes ev)
{
  struct bytes vlq = make_vlq(delta);       // convert delta into variable length quantity
  struct bytes b = byte_cat(vlq, ev, 1, 1);

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
  write_bytes(sig, 7, 0xFF, 0x58, 0x04, numer, denom_lg, 24, 8);
  
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
  write_bytes(tempo, 3, 0xFF, 0x51, 0x03);
  
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
