/* { dg-do compile } */
/* { dg-options "-O2" } */

int f (int);
unsigned int glob;

void
g ()
{
  while (f (glob));
  glob = 5;
}

/* { dg-final { scan-assembler-times "ldr" 2 } } */
