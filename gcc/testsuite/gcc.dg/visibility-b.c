/* { dg-do compile } */
/* { dg-require-visibility "" } */
/* { dg-final { scan-hidden "n" } } */

#define __private_extern__ extern __attribute__((visibility ("hidden")))

__private_extern__ int n;

int
mach_error_type(int sub)
{
 if (sub >= n)
     return 1;
 return 0;
}
