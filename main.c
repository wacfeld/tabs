#include <stdio.h>
#include <assert.h>

#include "midi.h"
#include "tabs.h"

int main()
{
  assert(sizeof(int) >= 4);
  assert(sizeof(long) >= 8);

  read_tabs(stdin, stdout);

  // struct bytes b = make_header(0,1,256);
  // put_bytes(stdout, b);

  // struct bytes sig = make_timesig(2, 4);
  // struct bytes tempo = make_tempo(80, 2, 4);

  // // struct bytes note = make_bytes(3, 0x99, 0x33, 0x59);
  // // struct bytes off = make_bytes(3, 0x89, 0x33, 0x59);
  // struct bytes end = make_bytes(3, 0xff, 0x2f, 0x00);

  // struct bytes note = make_midi_event(NOTE_ON, 9, 2, 0x27, 0x59);
  // struct bytes off = make_midi_event(NOTE_OFF, 9, 2, 0x27, 0x59);
  
  // struct bytes evs[] = {make_mtrk_event(0, sig), make_mtrk_event(0, tempo), make_mtrk_event(0, note), make_mtrk_event(0, off), make_mtrk_event(0, end)};
  
  // struct track tr = {5, -1, evs};
  // struct bytes chunk = make_track_chunk(tr);
  
  // put_bytes(stdout, chunk);
}
