/**
 * @file
 * Base64 encoder and decoder.
 * See base64.h for API specification.
 */

#include <stdio.h>

#include "libutils/base64.h"

#define B64_PAD '='

static char sym_table[] = {
// 0    1    2    3    4    5    6    7    8    9    a    b    c    d    e    f
  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
  'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
  'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
  'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'
};

static uint8_t
byte2block(char c)
{
  if (c == B64_PAD)
    return 0x00;
  if (c == '/')
    return 0x3f;
  if (c == '+')
    return 0x3e;
  if (c <= '9')
    return 0x34 + c - '0';
  if (c <= 'Z')
    return c - 'A';
  if (c <= 'z')
    return 0x1a + c - 'a';
  return 0;
}

char *
b64_encode(const uint8_t *buffer, size_t size, size_t *out_size)
{
  int i, j, pad;
  uint8_t b0, b1, b2;
  uint32_t bits;
  char *out_str;

  *out_size = ((4 * size / 3) + 3) & ~0x03;
  out_str = malloc(*out_size); // should this be a calloc of size out + 1
  if (out_str == NULL)
    return NULL;

  for (i = 0, j = 0, pad = 0; i < size;) {
    b0 = (i < size) ? buffer[i++] : 0;
    pad += (i < size) ? 0 : 1;
    b1 = (i < size) ? buffer[i++] : 0;
    pad += (i < size) ? 0 : 1;
    b2 = (i < size) ? buffer[i++] : 0;

    bits = (b0 << 0x10) + (b1 << 0x08) + b2;

    out_str[j++] = sym_table[(bits >> 18) & 0x3f];
    out_str[j++] = sym_table[(bits >> 12) & 0x3f];
    out_str[j++] = (pad < 2) ? sym_table[(bits >> 6) & 0x3f] : B64_PAD;
    out_str[j++] = (pad < 1) ? sym_table[bits & 0x3f] : B64_PAD;
  }
  return out_str;
}

uint8_t *
b64_decode(const char *b64_str, size_t size, size_t *out_size)
{
  uint8_t *out_buf;
  int i, j, pad;
  uint8_t c0, c1, c2, c3;
  uint32_t bits;

  *out_size = 3 * size / 4;
  if (b64_str[size - 1] == B64_PAD)
    (*out_size) -= 1;
  if (b64_str[size - 2] == B64_PAD)
    (*out_size) -= 1;

  out_buf = malloc(*out_size);
  if (out_buf == NULL)
    return NULL;

  for (i = 0, j = 0, pad = 0; i < size;) {
    c0 = b64_str[i++];
    c1 = b64_str[i++];
    c2 = b64_str[i++];
    pad += (c2 == B64_PAD) ? 1 : 0;
    c3 = b64_str[i++];
    pad += (c3 == B64_PAD) ? 1 : 0;

    bits = ((byte2block(c0) << 18) +
	    (byte2block(c1) << 12) +
	    (byte2block(c2) << 6) +
	    byte2block(c3));

    out_buf[j++] = (bits >> 0x10) & 0xff;
    if (pad < 2)
      out_buf[j++] = (bits >> 0x08) & 0xff;
    if (pad < 1) 
      out_buf[j++] = bits & 0xff;
  }

  return out_buf;
}
