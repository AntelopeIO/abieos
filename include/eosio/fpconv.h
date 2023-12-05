/// From https://github.com/night-shift/fpconv
/// Boost Software License 1.0
/// See accompanying license file

#ifndef FPCONV_H
#   define FPCONV_H

/* Fast and accurate double to string conversion based on Florian Loitsch's
 * Grisu-algorithm[1].
 *
 * Input:
 * fp -> the double to convert, dest -> destination buffer.
 * The generated string will never be longer than 24 characters plus a possible '-' character.
 * Make sure to pass a pointer to at least 25 bytes of memory.
 * The emitted string will not be null terminated.
 *
 * Output:
 * The number of written characters.
 *
 * Exemplary usage:
 *
 * void print(double d)
 * {
 *      char buf[25 + 1] // plus null terminator
 *      int str_len = fpconv_dtoa(d, buf);
 *
 *      buf[str_len] = '\0';
 *      printf("%s", buf);
 * }
 *
 */

#   ifdef __cplusplus
extern "C" int fpconv_dtoa(double fp, char dest[25]);
#   else
int fpconv_dtoa(double fp, char dest[25]);
#   endif

#endif

/* [1] http://florian.loitsch.com/publications/dtoa-pldi2010.pdf */
