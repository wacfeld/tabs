#ifndef MIDI_HEAD
#define MIDI_HEAD

typedef unsigned char uchar;
typedef unsigned int uint;

struct bytes
{
  int len;
  uchar *b;
};

struct track
{
  int n_evs;
  struct bytes *evs;
};

void put_bytes(struct bytes b, int discard);
void write_bytes(uchar *s, int n, ...);

struct bytes byte_cat(struct bytes x, struct bytes y, int reall, int discard);
struct bytes make_track_chunk(struct track tr);

struct bytes make_header(uint format, uint tracks, uint division);
struct bytes make_timesig(uchar numer, uchar denom);
struct bytes make_tempo(uint bpm, uchar numer, uchar denom);
  
struct bytes make_mtrk_event(uint delta, struct bytes ev);

struct bytes make_vlq(uint n);
#endif
