#include <stdio.h>
#include <assert.h>

#include "midi.h"
#include "tabs.h"

int main()
{
  assert(sizeof(int) >= 4);
  assert(sizeof(long) >= 8);

  read_tabs(stdin, stdout);
}
