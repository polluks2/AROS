/*
    (C) 1997-98 AROS - The Amiga Replacement OS
    $Id$
    
    Desc: Begining of AROS kernel
    Lang: English
*/

/*****************************************************************************

    FUNCTION
	This is the main file in Native PC AROS. The main function is called
	by head.S file. All this code is in flat 32-bit mode.
	
	This file will make up the exec base, memory and other stuff. We don't
	need any malloc functions, because there is no OS yet. To be honest:
	while main is running Native PC AROS is the only OS on our PC.

	Be careful. You cant use any stdio. Only your own functions are
	acceptable at this moment!!

*****************************************************************************/

#include "text.h"
#include "logo.h"

#define KERNEL_DATA (void *)0x98000
void show_status(void)
{
unsigned char *p;
int d;

  p = KERNEL_DATA;
  puts_fg("\nAROS detected Hardware\nProcessortype: 80");
  d = p[0]-1;
  puti_fg(d);
  puts_fg("86\nAvailable Memory: ");
  d = (p[3]<<8) + p[2];
  puti_fg(d);
  puts_fg("kB\n");

}

int main()
{
  char text[] = "Now booting AROS - The Amiga Replacement OS\n";

  showlogo();
  gotoxy(0,0);
  puts_fg(text);
  show_status();

return 0;
}
