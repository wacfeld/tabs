#include <stdio.h>
#include <assert.h>

#include "midi.h"

int main()
{
  assert(sizeof(int) >= 4);
  assert(sizeof(long) >= 8);

  put_bytes(make_header(0, 1, 256), 1);

  struct bytes sig = make_timesig(2, 4);
  struct bytes tempo = make_tempo(80, 2, 4);

  struct bytes note = make_bytes(3, 0x99, 0x33, 0x59);
  
  struct bytes evs[] = {make_mtrk_event(0, sig), make_mtrk_event(0, tempo), make_mtrk_event(0, note)};
  
  struct track tr = {3, evs};
  struct bytes chunk = make_track_chunk(tr, 0);
  
  put_bytes(chunk, 1);
}
