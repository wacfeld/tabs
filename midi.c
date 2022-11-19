#include <stdio.h>
#include <assert.h>

typedef unsigned char uchar;
typedef unsigned int uint;

struct bytes
{
  int len;
  char *b;
};

// initial header chunk; chunk type followed by length (6 bytes)
const uchar head[] = {'M', 'T', 'h', 'd', 0, 0, 0, 6};

// 16 bit format; 0 means single track
const uchar format[] = {0, 0};

// 16 bit track count
const uchar ntrks[] = {0, 1};

// 16 bit division (configurable)
uchar division[2];

void set_division(uint d)
{
  // must fit within (unsigned) 15 bits
  assert(d <= 32767);
  // due to this constraint the highest bit is 0, as required

  division[0] = d >> 8; // highest 8 bits
  division[1] = d;      // lowest 8 bits
}

// create variable length quantity from integer
// returns number of bytes (at most 4), writes bytes into vlq
int make_vlq(uint n, uchar *vlq)
{
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

  return len;
}

// create a time signature given a numerator and denominator (e.x. cut time 2, 4)
void make_timesig(uint numer, uint denom)
{
  int denom_lg = 0;
  // take the log base 2 of denom
  while(denom != 1)
  {
    denom_lg++;
    assert(denom % 2 == 0); // denom must be a power of 2
    denom /= 2;
  }

  
}
