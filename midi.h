#ifndef MIDI_HEAD
#define MIDI_HEAD

typedef unsigned char uchar;
typedef unsigned int uint;

struct bytes
{
  int len;
  uchar *b;
};

struct bytes make_header(uint format, uint tracks, uint division);
struct bytes make_timesig(uchar numer, uchar denom);
struct bytes make_tempo(uint bpm, uchar numer, uchar denom);
  
struct bytes make_vlq(uint n);
#endif
