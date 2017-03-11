/**
 * @file
 * Base64 encoder and decoder
 */

#ifndef BASE64_H
#define BASE64_H
#include <stdlib.h>
#include <stdint.h>

/**
 * Encode buffer to base64
 *
 * @param[in] buffer: input buffer
 * @param[in] size: size of the input buffer in bytes
 * @param[out] b64string: base64 null-terminated string
 * @return: the size of the base64 string
 */
char * b64_encode(const uint8_t *buffer, size_t size, size_t *out_size);

/**
 * Decode base64 string to buffer
 *
 * @param[in] b64string: input base64 null-terminated string
 * @param[out] buffer: output buffer
 * @return: the size of the output buffer or zero
 */
uint8_t * b64_decode(const char *b64string, size_t size, size_t *out_size);

#endif /* BASE64_H */
