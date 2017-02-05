
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <string.h>

#include "libutils/base64.h"


static void
test_encode(void **state)
{
  char *out;
  size_t len;
  
  out = b64_encode((uint8_t *)"", 0, &len);
  assert_int_equal(len, 0);
  
  out = b64_encode((uint8_t *)"1", 1, &len);
  assert_memory_equal(out, "MQ==", 4);
  assert_int_equal(len, 4);
  
  out = b64_encode((uint8_t *)"12", 2, &len);
  assert_memory_equal(out, "MTI=", 4);
  assert_int_equal(len, 4);
  
  out = b64_encode((uint8_t *)"123", 3, &len);
  assert_memory_equal(out, "MTIz", 4);
  assert_int_equal(len, 4);

  out = b64_encode((uint8_t *)"1234", 4, &len);
  assert_memory_equal(out, "MTIzNA==", 8);
  assert_int_equal(len, 8);

  out = b64_encode((uint8_t *)"12345", 5, &len);
  assert_memory_equal(out, "MTIzNDU=", 8);
  assert_int_equal(len, 8);

  out = b64_encode((uint8_t *)"123456", 6, &len);
  assert_memory_equal(out, "MTIzNDU2", 8);
  assert_int_equal(len, 8);

  out = b64_encode((uint8_t *)"1234567", 7, &len);
  assert_memory_equal(out, "MTIzNDU2Nw==", 12);
  assert_int_equal(len, 12);

  out = b64_encode((uint8_t *)"12345678", 8, &len);
  assert_memory_equal(out, "MTIzNDU2Nzg=", 12);
  assert_int_equal(len, 12);

  out = b64_encode((uint8_t *)"123456789", 9, &len);
  assert_memory_equal(out, "MTIzNDU2Nzg5", 12);
  assert_int_equal(len, 12);
}

static void
test_decode(void **state)
{
  uint8_t *out;
  size_t len;
  
  out = b64_decode("", 0, &len);
  assert_int_equal(len, 0);
  
  out = b64_decode("MQ==", 4, &len);
  assert_memory_equal(out, "1", 1);
  assert_int_equal(len, 1);
  
  out = b64_decode("MTI=", 4, &len);
  assert_memory_equal(out, "12", 2);
  assert_int_equal(len, 2);
  
  out = b64_decode("MTIz", 4, &len);
  assert_memory_equal(out, "123", 3);
  assert_int_equal(len, 3);

  out = b64_decode("MTIzNA==", 8, &len);
  assert_memory_equal(out, "1234", 4);
  assert_int_equal(len, 4);

  out = b64_decode("MTIzNDU=", 8, &len);
  assert_memory_equal(out, "12345", 5);
  assert_int_equal(len, 5);

  out = b64_decode("MTIzNDU2", 8, &len);
  assert_memory_equal(out, "123456", 6);
  assert_int_equal(len, 6);

  out = b64_decode("MTIzNDU2Nw==", 12, &len);
  assert_memory_equal(out, "1234567", 7);
  assert_int_equal(len, 7);

  out = b64_decode("MTIzNDU2Nzg=", 12, &len);
  assert_memory_equal(out, "12345678", 8);
  assert_int_equal(len, 8);

  out = b64_decode("MTIzNDU2Nzg5", 12, &len);
  assert_memory_equal(out, "123456789", 9);
  assert_int_equal(len, 9);
}

int
main(int argc, char *argv[])
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_encode),
    cmocka_unit_test(test_decode),
  };
  return cmocka_run_group_tests(tests, NULL, NULL);
}
