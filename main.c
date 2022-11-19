#include <stdio.h>
#include <assert.h>

int main()
{
  uchar vlq[4] = {0};
  int x = 0;
  scanf("%x", &x);
  int len = make_vlq(x, vlq);
  printf("%d %02x %02x %02x %02x\n", len, vlq[0], vlq[1], vlq[2], vlq[3]);
}
