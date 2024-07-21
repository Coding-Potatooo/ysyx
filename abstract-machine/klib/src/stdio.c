#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

/*
RETURN VALUE
       Upon successful return, these functions return the number of characters printed (excluding the null byte used to end output to strings).

       The functions snprintf() and vsnprintf() do not write more than size bytes (including the terminating null byte ('\0')).  If the output was truncated due to this limit, then the
       return value is the number of characters (excluding the terminating null byte) which would have been written to the final string if enough space had been available.  Thus, a re‐
       turn value of size or more means that the output was truncated.  (See also below under NOTES.)

       If an output error is encountered, a negative value is returned.
*/
int printf(const char *fmt, ...)
{
  panic("Not implemented");
}

int vsprintf(char *out, const char *fmt, va_list ap)
{
  panic("Not implemented");
}

int sprintf(char *out, const char *fmt, ...)
{
  /*man stdarg*/
  va_list args;
  va_start(args, fmt);
  int i_fmt = 0, i_out = 0;
  while (fmt[i_fmt])
  {
    if ((fmt[i_fmt]) == '%')
    {
      switch (fmt[++i_fmt])
      {
      case 's':
      {
        i_fmt += 1; // go over the s following %
        char *s = va_arg(args, char *);
        for (int i = 0; s[i]; i++)
        {
          out[i_out++] = s[i];
        }
        // should not print 0 to out.
        break;
      }
      case 'd':
      {
        i_fmt += 1; // go over the d following %
        int d = va_arg(args, int);
        if (d < 0)
        {
          out[i_out++] = '-';
          d = -d;
        }
        else if (d == 0)
        {
          out[i_out++] = '0';
        }
        char buf[100];
        int i_buf = 0;
        while (d)
        {
          buf[i_buf++] = (d % 10) + '0';
          d /= 10;
        }
        for (int i = i_buf - 1; i >= 0; i--)
        {
          out[i_out++] = buf[i];
        }
        break;
      }
      default:
        // to implement more functionalities.
        assert(0);
        break;
      }
    }
    else
    {
      out[i_out++] = fmt[i_fmt++];
    }
  }
  out[i_out++] = '\0';
  va_end(args);

  return i_out;
}

int snprintf(char *out, size_t n, const char *fmt, ...)
{
  panic("Not implemented");
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap)
{
  panic("Not implemented");
}

#endif
