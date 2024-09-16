#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s)
/*
The strlen() function calculates the length of the string pointed to by s,
excluding the terminating null byte ('\0').
*/
{
  int len = 0;
  for (len = 0; *(s + len); len++)
    ;
  return len;
}

char *strcpy(char *dst, const char *src)
/*
The  strcpy()  function  copies the string pointed to by src, including
the terminating null byte ('\0'), to the buffer  pointed  to  by  dest.
The  strings  may  not overlap, and the destination string dest must be
large enough to receive the copy.  Beware  of  buffer  overruns!   (See
BUGS.)
return: The strcpy() and strncpy() functions return a pointer to  the  destina‐
       tion string dest.
*/
{

  int n = strlen(src);
  if (!((char *)src > (char *)dst + n || (char *)src + n < (char *)dst))
  {
    assert(0); // assert(0) when src,dst memory area overlaps
  }
  for (int i = 0; i < n + 1 /*to include the /0 of src*/; i++)
  {
    *(dst + i) = *(src + i);
  }
  return dst;
}

char *strncpy(char *dst, const char *src, size_t n)
/*
The  strncpy()  function is similar, except that at most n bytes of src are copied.
Warning: If there is no null byte among the first n  bytes
of src, the string placed in dest will not be null-terminated.

return: The strcpy() and strncpy() functions return a pointer to  the  destination string dest.
*/
{
  if (!((char *)src > (char *)dst + n || (char *)src + n < (char *)dst))
  {
    assert(0); // assert(0) when src,dst memory area overlaps
  }
  int len = strlen(src);
  int min = len < n ? len : n;
  for (int i = 0; i < min; i++)
  {
    *(dst + i) = *(src + i);
  }
  for (int i = 0; i < n - min; i++)
  {
    *(dst + i) = '\0';
  }
  return dst;
}

char *strcat(char *dst, const char *src)
/*
The  strcat() function appends the src string to the dest string, over‐
writing the terminating null byte ('\0') at the end of dest,  and  then
adds  a  terminating  null  byte.  The strings may not overlap, and the
dest string must have enough space for the  result.   If  dest  is  not
large  enough, program behavior is unpredictable; buffer overruns are a
favorite avenue for attacking secure programs.

return: The strcat() and strncat() functions return a pointer to the  resulting
       string dest.
*/
{
  int i = 0;
  for (i = 0; *(dst + i); i++)
    ;
  int src_len = strlen(src);
  for (int j = 0; j < src_len + 1 /*to include the '/0' at the end of src*/; j++)
  {
    *(dst + i + j) = *(src + j);
  }
  return dst;
}

int strcmp(const char *s1, const char *s2)
/*
strcmp() returns an integer indicating the result of the comparison, as follows:
       • 0, if the s1 and s2 are equal;
       • a negative value if s1 is less than s2;
       • a positive value if s1 is greater than s2.

       The strncmp() function is similar, except it compares  only  the  first
       (at most) n bytes of s1 and s2.

return:  The strcmp() and strncmp() functions return an integer less than, equal
  to, or greater than zero if s1 (or the first n bytes thereof) is found,  respectively, to be less than, to match, or be greater than s2.
*/
{
  int i = 0;
  for (i = 0; *(s1 + i) || *(s2 + i); i++)
  {
    if (*(s1 + i) != *(s2 + i))
    {
      return *(s1 + i) - *(s2 + i);
    }
  }
  return (*(s1 + i) - *(s2 + i));
}

int strncmp(const char *s1, const char *s2, size_t n)
{
  int i = 0;
  for (i = 0; (*(s1 + i) || *(s2 + i)) && i < n - 1; i++)
  {
    if (*(s1 + i) != *(s2 + i))
    {
      return *(s1 + i) - *(s2 + i);
    }
  }
  return (*(s1 + i) - *(s2 + i));
}

void *memset(void *s, int c, size_t n)
/*
DESCRIPTION
       The memset() function fills the first n bytes of the memory area pointed to by s with the constant byte c.

RETURN VALUE
       The memset() function returns a pointer to the memory area s.
*/
{
  for (int i = 0; i < n; i++)
  {
    *((char *)s + i) = (char)c;
  }
  return s;
}

void *memmove(void *dst, const void *src, size_t n)
/*
DESCRIPTION
       The  memmove()  function  copies n bytes from memory area src to memory
       area dest.  The memory areas may overlap: copying takes place as though
       the  bytes in src are first copied into a temporary array that does not
       overlap src or dest, and the bytes are then copied from the temporary
       array to dest.

RETURN VALUE
       The memmove() function returns a pointer to dest.
*/
{
  if (dst < src)
  {
    for (int i = 0; i < n; i++)
    {
      *((char *)dst + i) = *((char *)src + i);
    }
  }
  else if (dst > src)
  {
    for (int i = n - 1; i >= 0; i--)
    {
      *((char *)dst + i) = *((char *)src + i);
    }
  }
  return dst;
}

void *memcpy(void *dst, const void *src, size_t n)
/*
DESCRIPTION
       The  memcpy()  function  copies  n  bytes from memory area src to memory area dest.  The memory areas must not overlap.
       Use memmove(3) if the memory areas do overlap.

RETURN VALUE
       The memcpy() function returns a pointer to dest.
*/
{
  if (!((char *)src > (char *)dst + n || (char *)src + n < (char *)dst))
  {
    assert(0); // assert(0) when src,dst memory area overlaps
  }
  for (int i = 0; i < n; i++)
  {
    *((char *)dst + i) = *((char *)src + i);
  }
  return dst;
}

int memcmp(const void *s1, const void *s2, size_t n)
/*
DESCRIPTION
       The memcmp() function compares the first n bytes (each interpreted as unsigned char) of the memory areas s1 and s2.

RETURN VALUE
       The  memcmp()  function  returns an integer less than, equal to, or greater than zero if the first n bytes of s1 is found, re‐
       spectively, to be less than, to match, or be greater than the first n bytes of s2.

       For a nonzero return value, the sign is determined by the sign of the difference between the first pair of bytes  (interpreted
       as unsigned char) that differ in s1 and s2.

       If n is zero, the return value is zero.
*/
{
  int i = 0;
  for (i = 0; i < n - 1; i++)
  {
    if (*((char *)s1 + i) != *((char *)s2 + i))
    {
      return *((char *)s1 + i) - *((char *)s2 + i);
    }
  }
  return *((char *)s1 + i) - *((char *)s2 + i);
}

#endif