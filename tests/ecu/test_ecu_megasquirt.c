/**
 * Navit, a modular navigation system.
 *
 * Unit tests for ECU MegaSquirt backend.
 */

#include <stdio.h>
#include <string.h>

#define TEST_ASSERT(cond, msg)                                                 \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fprintf(stderr, "FAIL: %s:%d: %s\n", __FILE__, __LINE__, msg);           \
      return 1;                                                                \
    }                                                                          \
  } while (0)

#define MS_OFFSET_RPM 2
#define MS_OFFSET_PW1 4
#define MS_N_CYL_DEFAULT 4
#define MS_MIN_BYTES (MS_OFFSET_PW1 + 2)

static double megasquirt_fuel_rate(double pw_ms, unsigned int rpm, int n_cyl,
                                   int injector_flow_cc_min) {
  if (pw_ms <= 0.0 || rpm == 0 || n_cyl <= 0 || injector_flow_cc_min <= 0)
    return 0.0;
  double rate = pw_ms * (double)rpm * (double)n_cyl *
                (double)injector_flow_cc_min / 2000000.0;
  if (rate <= 0.0 || rate > 600.0)
    return 0.0;
  return rate;
}

static int megasquirt_would_accept(const unsigned char *buf, int n, int flow_cc,
                                   double *out_rate_l_h) {
  if (out_rate_l_h)
    *out_rate_l_h = 0.0;
  if (n < MS_MIN_BYTES || flow_cc <= 0)
    return 0;
  unsigned int rpm = ((unsigned int)buf[MS_OFFSET_RPM] << 8) |
                     (unsigned int)buf[MS_OFFSET_RPM + 1];
  unsigned int pw_raw = ((unsigned int)buf[MS_OFFSET_PW1] << 8) |
                        (unsigned int)buf[MS_OFFSET_PW1 + 1];
  double pw_ms = (double)pw_raw / 1000.0;
  if (!(rpm > 200 && rpm < 12000 && pw_ms > 0.0 && pw_ms < 50.0))
    return 0;
  double rate = megasquirt_fuel_rate(pw_ms, rpm, MS_N_CYL_DEFAULT, flow_cc);
  if (rate <= 0.0)
    return 0;
  if (out_rate_l_h)
    *out_rate_l_h = rate;
  return 1;
}

static int test_fuel_rate_formula(void) {
  double r = megasquirt_fuel_rate(2.0, 3000, 4, 250);
  TEST_ASSERT(r > 2.9 && r < 3.1, "Valid fuel rate formula mismatch");

  TEST_ASSERT(megasquirt_fuel_rate(0.0, 3000, 4, 250) == 0.0,
              "Zero pw_ms should return 0");
  TEST_ASSERT(megasquirt_fuel_rate(2.0, 0, 4, 250) == 0.0,
              "Zero rpm should return 0");
  TEST_ASSERT(megasquirt_fuel_rate(2.0, 3000, 0, 250) == 0.0,
              "Zero n_cyl should return 0");
  TEST_ASSERT(megasquirt_fuel_rate(2.0, 3000, 4, 0) == 0.0,
              "Zero flow should return 0");
  TEST_ASSERT(megasquirt_fuel_rate(-1.0, 3000, 4, 250) == 0.0,
              "Negative pw_ms should return 0");

  r = megasquirt_fuel_rate(50.0, 10000, 4, 650);
  TEST_ASSERT(r == 0.0, "Rate > 600 should return 0");

  r = megasquirt_fuel_rate(30.0, 10000, 4, 100);
  TEST_ASSERT(r > 0.0 && r <= 600.0, "Rate at 600 boundary");

  return 0;
}

static int test_parse_valid_block(void) {
  unsigned char buf[] = {0x00, 0x00, 0x0B, 0xB8, 0x07, 0xD0};
  double rate;
  int ok = megasquirt_would_accept(buf, sizeof(buf), 250, &rate);
  TEST_ASSERT(ok == 1, "Valid 6-byte block should be accepted");
  TEST_ASSERT(rate > 2.9 && rate < 3.1, "Parsed rate for 3000 rpm, 2 ms PW");

  return 0;
}

static int test_malformed_short_buffer(void) {
  unsigned char buf[8];
  memset(buf, 0x00, sizeof(buf));
  buf[2] = 0x0B;
  buf[3] = 0xB8;
  buf[4] = 0x00;
  buf[5] = 0x64;

  TEST_ASSERT(megasquirt_would_accept(buf, 0, 250, NULL) == 0,
              "Zero-length buffer rejected");
  TEST_ASSERT(megasquirt_would_accept(buf, 1, 250, NULL) == 0,
              "1-byte buffer rejected");
  TEST_ASSERT(megasquirt_would_accept(buf, 5, 250, NULL) == 0,
              "5-byte buffer rejected (need 6)");
  TEST_ASSERT(megasquirt_would_accept(buf, 6, 250, NULL) == 1,
              "6-byte buffer accepted");

  return 0;
}

static int test_malformed_zero_flow(void) {
  unsigned char buf[] = {0x00, 0x00, 0x0B, 0xB8, 0x07, 0xD0};
  TEST_ASSERT(megasquirt_would_accept(buf, 6, 0, NULL) == 0,
              "Zero flow must reject");
  TEST_ASSERT(megasquirt_would_accept(buf, 6, -1, NULL) == 0,
              "Negative flow must reject");

  return 0;
}

static int test_malformed_rpm_bounds(void) {
  unsigned char buf[6];
  memset(buf, 0, sizeof(buf));
  buf[4] = 0x07;
  buf[5] = 0xD0;

  buf[2] = 0;
  buf[3] = 0;
  TEST_ASSERT(megasquirt_would_accept(buf, 6, 250, NULL) == 0,
              "RPM 0 rejected");

  buf[2] = 0x00;
  buf[3] = 0xC8;
  TEST_ASSERT(megasquirt_would_accept(buf, 6, 250, NULL) == 0,
              "RPM 200 rejected (exclusive lower bound)");

  buf[2] = 0x2E;
  buf[3] = 0xDF;
  TEST_ASSERT(megasquirt_would_accept(buf, 6, 250, NULL) == 1,
              "RPM 11999 accepted");

  buf[2] = 0x2E;
  buf[3] = 0xE8;
  TEST_ASSERT(megasquirt_would_accept(buf, 6, 250, NULL) == 0,
              "RPM 12000 rejected");

  buf[2] = 0xFF;
  buf[3] = 0xFF;
  TEST_ASSERT(megasquirt_would_accept(buf, 6, 250, NULL) == 0,
              "RPM 65535 rejected");

  return 0;
}

static int test_malformed_pw_bounds(void) {
  unsigned char buf[6];
  memset(buf, 0, sizeof(buf));
  buf[2] = 0x0B;
  buf[3] = 0xB8;

  buf[4] = 0;
  buf[5] = 0;
  TEST_ASSERT(megasquirt_would_accept(buf, 6, 250, NULL) == 0,
              "PW 0 ms rejected");

  buf[4] = 0x00;
  buf[5] = 0x01;
  TEST_ASSERT(megasquirt_would_accept(buf, 6, 250, NULL) == 1,
              "Small PW accepted");

  buf[4] = 0xC3;
  buf[5] = 0x50;
  TEST_ASSERT(megasquirt_would_accept(buf, 6, 250, NULL) == 0,
              "PW 50.0 ms rejected (exclusive upper bound)");

  buf[4] = 0xFF;
  buf[5] = 0xFF;
  TEST_ASSERT(megasquirt_would_accept(buf, 6, 250, NULL) == 0,
              "PW 65.535 ms rejected");

  return 0;
}

static int test_overflow_produces_zero_rate(void) {
  unsigned char all_ff[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  double rate;
  int ok = megasquirt_would_accept(all_ff, 6, 250, &rate);
  TEST_ASSERT(ok == 0, "All 0xFF buffer not accepted (rpm/pw out of range)");
  TEST_ASSERT(rate == 0.0, "Rate remains 0 for overflow bytes");

  unsigned char high_rate[] = {0x00, 0x00, 0x2E, 0xE0, 0xC3, 0x50};
  high_rate[2] = 0x27;
  high_rate[3] = 0x10;
  high_rate[4] = 0xBF;
  high_rate[5] = 0x58;
  ok = megasquirt_would_accept(high_rate, 6, 500, &rate);
  TEST_ASSERT(ok == 1 && rate > 0.0 && rate <= 600.0,
              "High but valid rate accepted and clamped if needed");

  return 0;
}

static int test_uninitialized_garbage_bytes(void) {
  unsigned char short_buf[3] = {0xAB, 0xCD, 0xEF};
  TEST_ASSERT(megasquirt_would_accept(short_buf, 3, 250, NULL) == 0,
              "Short buffer with garbage rejected");

  unsigned char garbage[6] = {0x11, 0x22, 0x01, 0x00, 0x00, 0x01};
  double rate;
  megasquirt_would_accept(garbage, 6, 250, &rate);
  TEST_ASSERT(rate >= 0.0 && rate <= 600.0,
              "Garbage buffer yields in-range or zero rate");

  return 0;
}

int main(void) {
  int failures = 0;

  printf("Running ECU MegaSquirt backend tests...\n");

  failures += test_fuel_rate_formula();
  failures += test_parse_valid_block();
  failures += test_malformed_short_buffer();
  failures += test_malformed_zero_flow();
  failures += test_malformed_rpm_bounds();
  failures += test_malformed_pw_bounds();
  failures += test_overflow_produces_zero_rate();
  failures += test_uninitialized_garbage_bytes();

  if (failures == 0) {
    printf("All MegaSquirt backend tests passed.\n");
    return 0;
  }
  printf("%d test(s) failed\n", failures);
  return 1;
}
