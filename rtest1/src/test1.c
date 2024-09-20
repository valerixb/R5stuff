//
// first R5 demo application
//
// latest rev: sept 20 2024
//

#include <stdio.h>
#include "platform.h"
//#include "xil_printf.h"

#define REGBANK (volatile unsigned int *)(0x80020000)

int main()
  {
  unsigned int thereg, theval;

  init_platform();

  printf("R5 test application #1\n");

  setvbuf (stdout, NULL, _IONBF, 0);
  setvbuf (stdin, NULL, _IONBF, 0);
  
  while(1)
    {
    printf("\nInput register number to read (0..15)\n");
    scanf("%u",&thereg);
    if(thereg>15) break;
    theval=*(REGBANK+thereg);
    printf("Reg #%02u is 0x%08X\n", thereg, theval);
    }

  cleanup_platform();
  return 0;
  }
