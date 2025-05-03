#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>
#include <stdint.h>
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
  /*
  如何减少代码重复度？这里重复的代码主要是解析格式的部分...
  已经实现了一个 sprintf 函数...那么 printf 直接调用 sprintf 就可以了。
  困难是，怎么把printf ...的参数传递给sprintf函数呢？
  实际上需要先实现vsprintf函数，printf和sprintf函数再调用vsprintf函数。
  */
  va_list args;
  va_start(args, fmt);

  char tmp_buf[1024];
  int len = vsprintf(tmp_buf, fmt, args);
  for (int i = 0; i < len; i++) {
    putch(tmp_buf[i]);
  }
  free(tmp_buf);
  va_end(args);
  return len;
}

int vsprintf(char *out, const char *fmt, va_list ap)
{
  size_t max_size = SIZE_MAX;
  return vsnprintf(out, max_size, fmt, ap);
}

int sprintf(char *out, const char *fmt, ...)
/*
怎么调试klib的代码？
运行在native上！make ARCH=native gdb

或者直接粘贴代码到本地，在本地IDE上调试
*/
{
  va_list args;
  va_start(args, fmt);
  int len = vsprintf(out, fmt, args);
  va_end(args);
  return len;
}

int snprintf(char *out, size_t n, const char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  int len = vsnprintf(out, n, fmt, args);
  va_end(args);
  return len;
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap)
{
  /*
  man stdarg
  va_list 是 C 语言标准库 <stdarg.h> 中定义的一个类型，用于支持可变参数函数（Variable Argument Function）。
  简单说，就是如果一个函数的参数数量不确定（比如 printf），就可以用 va_list 来遍历这些参数。
  */


  /*
  从vsprintf迁移到vsnprintf:
  你需要在所有 out[i_out++] = ... 前加上判断：
    if (i_out < n - 1) {
        out[i_out++] = ...;
    } else {
        i_out++;  // 虽然没写入，但仍增加总长度
    }

  为了不重复写判断，你可以定义一个宏或函数：
    #define PUTCHAR(ch) do { \
      if (i_out < n - 1) out[i_out] = (ch); \
      i_out++; \
    } while (0)

  然后把所有 out[i_out++] = ...; 改为：
    PUTCHAR(...);
  */
  
  #define PUTCHAR(ch) do { \
    if (i_out < n - 1) {out[i_out] = (ch); \
    i_out++;} \
  } while (0)


  int i_fmt = 0, i_out = 0;
  while (fmt[i_fmt])
  {
    /*
      %[flags][width][.precision][length]specifier

      处理格式化字符串
      %之后的格式化规则非常多...
      如果用状态机来处理的话，感觉代码会非常复杂...
      或者用文法来处理？细节记不清了...
    */
    if (fmt[i_fmt] == '%') {
      i_fmt++;  // skip '%'
      // 1. Parse flags
      char pad_char = ' ';  // 默认用空格填充
      int left_align = 0;
      if (fmt[i_fmt] == '0') {
          pad_char = '0';
          i_fmt++;
      } else if (fmt[i_fmt] == '-') {
          left_align = 1;
          i_fmt++;
      }
  
      // 2. Parse width
      int width = 0;
      while (fmt[i_fmt] >= '0' && fmt[i_fmt] <= '9') {
          width = width * 10 + (fmt[i_fmt++] - '0');
      }
  
      // 3. Handle format specifiers
      switch (fmt[i_fmt++]) {
      case 's': {
          char *s = va_arg(ap, char *);
          int len = 0;
          while (s[len]) len++;
  
          int padding = (width > len) ? (width - len) : 0;
  
          if (!left_align) {
              for (int i = 0; i < padding; ++i)
                  // out[i_out++] = pad_char;
                  PUTCHAR(pad_char);
          }
  
          for (int i = 0; i < len; ++i)
              // out[i_out++] = s[i];
              PUTCHAR(s[i]);
  
          if (left_align) {
              for (int i = 0; i < padding; ++i)
                  // out[i_out++] = pad_char;
                  PUTCHAR(pad_char);
          }
          break;
      }
      case 'd': {
          int d = va_arg(ap, int);
          char buf[100];
          int neg = 0, i_buf = 0;
  
          if (d < 0) {
              neg = 1;
              d = -d;
          }
  
          if (d == 0) {
              buf[i_buf++] = '0';
          } else {
              while (d) {
                  buf[i_buf++] = (d % 10) + '0';
                  d /= 10;
              }
          }
  
          int total_len = i_buf + neg;
          int padding = (width > total_len) ? (width - total_len) : 0;
  
          if (!left_align && pad_char != '0') {
              for (int i = 0; i < padding; ++i)
                  // out[i_out++] = pad_char;
                  PUTCHAR(pad_char);
          }
  
          if (neg) {
              // out[i_out++] = '-';
              PUTCHAR('-');
          }
  
          if (!left_align && pad_char == '0') {
              for (int i = 0; i < padding; ++i)
                  // out[i_out++] = '0';
                  PUTCHAR('0');
          }
  
          for (int i = i_buf - 1; i >= 0; --i)
              // out[i_out++] = buf[i];
              PUTCHAR(buf[i]);
  
          if (left_align) {
              for (int i = 0; i < padding; ++i)
                  // out[i_out++] = ' ';
                  PUTCHAR(' ');
          }
  
          break;
      }
      default:
          assert(0);  // 未支持的格式
      }
  }
  
    else
    {
      out[i_out++] = fmt[i_fmt++];
    }
  }
  PUTCHAR('\0');

  return i_out;
}

#endif
