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

enum status {NOTE_OFF=8, NOTE_ON=9,};

void put_bytes(FILE *stream, struct bytes b);
void write_bytes(uchar *s, int n, ...);
struct bytes make_bytes(int n, ...);

struct bytes byte_cat(struct bytes x, struct bytes y);
struct bytes make_track_chunk(struct track tr);

struct bytes make_header(uint format, uint tracks, uint division);
struct bytes make_timesig(uchar numer, uchar denom);
struct bytes make_tempo(uint bpm, uchar numer, uchar denom);
  
struct bytes make_mtrk_event(uint delta, struct bytes ev);
struct bytes make_midi_event(enum status stat, uint channel, int ndat, uchar dat1, uchar dat2);

struct bytes make_vlq(uint n);
#endif
