#include <stdio.h>
#include <assert.h>

typedef unsigned char uchar;
typedef unsigned int uint;

// initial header chunk
const uchar head[] = {'M', 'T', 'h', 'd', 0, 0, 0, 6};

// 16 bit format
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
  division[1] = d; // lowest 8 bits
}

// create variable length quantity from integer
char *make_vlc(uint n)
{
  
}

int main()
{
  assert(sizeof(int) >= 4);
}
