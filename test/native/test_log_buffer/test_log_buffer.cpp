// FROZEN TESTS (Contract V1.5.15) – Sprint P1 must make these pass. Do not modify.
// Host test of the REAL src/comm/log_buffer.cpp with test/native/freertos_shim
// (Serial capture, portMUX = std::mutex with critical-section depth tracking).
#define ADD_HOST_SHIM 1
#include <unity.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <thread>
#include <vector>
#include "comm/log_buffer.cpp"   // implementation under test

static char B[LOG_LINES * LOG_LINE_LEN + 64];

void setUp() { Serial.clear(); }
void tearDown() {}

static int countLines(const char* s) { int n = 0; for (; *s; ++s) n += (*s == '\n'); return n; }
static void fill(const char* tag) { for (int i = 0; i < LOG_LINES; ++i) logLine("%s%02d", tag, i); }

// must run first: nothing logged yet
void test_empty_at_start() {
    memset(B, 'x', sizeof B);
    TEST_ASSERT_EQUAL_UINT32(0, logDump(B, sizeof B));
    TEST_ASSERT_EQUAL_STRING("", B);
}
void test_single_line_and_printf_format() {
    logLine("value %d %s %.1f", 42, "mm", 1.25);
    logDump(B, sizeof B);
    size_t n = strlen(B);
    TEST_ASSERT_TRUE(n >= 14);
    TEST_ASSERT_EQUAL_STRING("value 42 mm 1.2\n", B + n - 16);     // last line, '\n' separated
}
void test_ring_keeps_newest_in_order() {
    fill("a");
    for (int i = 0; i < 5; ++i) logLine("b%02d", i);
    size_t n = logDump(B, sizeof B);
    TEST_ASSERT_EQUAL_UINT32(strlen(B), n);
    TEST_ASSERT_EQUAL_INT(LOG_LINES, countLines(B));
    std::string exp;
    for (int i = 5; i < LOG_LINES; ++i) { char l[16]; snprintf(l, sizeof l, "a%02d\n", i); exp += l; }
    for (int i = 0; i < 5; ++i) { char l[16]; snprintf(l, sizeof l, "b%02d\n", i); exp += l; }
    TEST_ASSERT_EQUAL_STRING(exp.c_str(), B);
}
void test_long_line_truncated() {
    fill("t");
    std::string longs(300, 'L');
    logLine("%s", longs.c_str());
    logDump(B, sizeof B);
    std::string exp(LOG_LINE_LEN - 1, 'L'); exp += "\n";
    size_t n = strlen(B);
    TEST_ASSERT_EQUAL_STRING(exp.c_str(), B + n - exp.size());
    TEST_ASSERT_EQUAL_CHAR('\n', B[n - exp.size() - 1]);              // previous line ends before
}
void test_one_trailing_newline_removed() {
    fill("n");
    logLine("abc\n");
    logLine("def\n\n");
    logDump(B, sizeof B);
    size_t n = strlen(B);
    TEST_ASSERT_EQUAL_STRING("abc\ndef\n\n", B + n - 9);               // only ONE '\n' removed
}
void test_echo_to_serial_outside_critical_section() {
    logLine("echo %d", 7);
    TEST_ASSERT_NOT_NULL(strstr(Serial.out.c_str(), "echo 7\r\n"));    // Serial.println
    TEST_ASSERT_EQUAL_INT(0, Serial.writes_in_critical);
}
void test_dump_small_buffer_keeps_newest_complete_lines() {
    fill("s");                                   // 20 lines "sNN\n" = 4 chars each
    char small[13];                              // room for 12 chars = 3 lines
    size_t n = logDump(small, sizeof small);
    TEST_ASSERT_EQUAL_UINT32(12, n);
    TEST_ASSERT_EQUAL_STRING("s17\ns18\ns19\n", small);
    char tiny[4];                                // 3 chars: not even one line fits
    TEST_ASSERT_EQUAL_UINT32(0, logDump(tiny, sizeof tiny));
    TEST_ASSERT_EQUAL_STRING("", tiny);
    char five[5];                                // exactly one line fits
    TEST_ASSERT_EQUAL_UINT32(4, logDump(five, sizeof five));
    TEST_ASSERT_EQUAL_STRING("s19\n", five);
}
void test_dump_small_buffer_lines_of_different_length() {
    fill("s");                                   // s00..s19 ...
    logLine("%s", "LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL");   // ... and one 49-char line
    char b[60];                                  // 59 chars: 49+1 and two short lines (2 x 4)
    size_t n = logDump(b, sizeof b);
    TEST_ASSERT_EQUAL_UINT32(58, n);
    TEST_ASSERT_EQUAL_STRING("s18\ns19\nLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL\n", b);
}
void test_each_call_uses_the_critical_section() {
    int before = shimCritEnters();
    logLine("x");
    TEST_ASSERT_EQUAL_INT(before + 1, shimCritEnters());   // exactly one section per line
    char b[64];
    before = shimCritEnters();
    logDump(b, sizeof b);
    TEST_ASSERT_TRUE(shimCritEnters() > before);           // the ring is read under the lock
    TEST_ASSERT_EQUAL_INT(0, shimCritDepth());              // and left again
}
void test_dump_cap_zero() {
    char c = 'x';
    TEST_ASSERT_EQUAL_UINT32(0, logDump(&c, 0));
    TEST_ASSERT_EQUAL_CHAR('x', c);              // nothing written
}
void test_parallel_logging_and_dumping() {
    for (int i = 0; i < LOG_LINES; ++i) logLine("T9-%05d-xxxxxxxxxxxxxxxx", i);   // same line format
    std::vector<std::thread> th;
    for (int t = 0; t < 4; ++t)
        th.emplace_back([t] { for (int i = 0; i < 3000; ++i) logLine("T%d-%05d-xxxxxxxxxxxxxxxx", t, i); });
    static char d[sizeof B];
    int bad = 0, lines = 0;
    for (int k = 0; k < 300; ++k) {
        logDump(d, sizeof d);
        const char* s = d;
        while (*s) {                              // every line complete: "Tt-nnnnn-" + 16 x
            const char* e = strchr(s, '\n');
            if (!e) { ++bad; break; }
            if ((int)(e - s) != 25 || s[0] != 'T' || s[24] != 'x') ++bad;
            ++lines;
            s = e + 1;
        }
    }
    for (auto& x : th) x.join();                  // assert only after all threads ended
    TEST_ASSERT_EQUAL_INT(0, bad);
    TEST_ASSERT_EQUAL_INT(300 * LOG_LINES, lines);
    logDump(d, sizeof d);
    TEST_ASSERT_EQUAL_INT(LOG_LINES, countLines(d));
    TEST_ASSERT_EQUAL_INT(0, Serial.writes_in_critical);
}

// ---- V1.5.15: review of P1.1 - logDump must copy the TEXT of the ring under the lock ----
// Lines of different length whose content encodes its own length ("<len>:" + filler). A dump
// that reads the text outside the critical section mixes old and new lines of other lengths.
static void lenLine(int len, char c) {
    char b[LOG_LINE_LEN];
    int h = snprintf(b, sizeof b, "%02d:", len);
    for (int k = h; k < len; ++k) b[k] = c;
    b[len] = 0;
    logLine("%s", b);
}
static bool consistent(const char* s, const char* e) {
    int len = (int)(e - s);
    if (len < 4 || s[2] != ':') return false;
    if ((s[0] - '0') * 10 + (s[1] - '0') != len) return false;
    for (const char* q = s + 3; q < e; ++q) if (*q != s[3]) return false;   // one filler char per line
    return true;
}
void test_v1515_dump_never_mixes_lines() {
    for (int i = 0; i < LOG_LINES; ++i) lenLine(10 + i % 40, 'a');
    std::vector<std::thread> th;
    for (int t = 0; t < 4; ++t)
        th.emplace_back([t] { for (int i = 0; i < 20000; ++i) lenLine(10 + (i * 7 + t * 13) % 80, (char)('b' + t)); });
    static char d[sizeof B];
    int bad = 0;
    for (int k = 0; k < 4000; ++k) {
        size_t n = logDump(d, sizeof d);
        if (n != strlen(d)) { ++bad; continue; }
        const char* s = d;
        while (*s) {
            const char* e = strchr(s, '\n');
            if (!e || !consistent(s, e)) { ++bad; break; }
            s = e + 1;
        }
    }
    for (auto& x : th) x.join();
    TEST_ASSERT_EQUAL_INT(0, bad);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_empty_at_start);
    RUN_TEST(test_single_line_and_printf_format);
    RUN_TEST(test_ring_keeps_newest_in_order);
    RUN_TEST(test_long_line_truncated);
    RUN_TEST(test_one_trailing_newline_removed);
    RUN_TEST(test_echo_to_serial_outside_critical_section);
    RUN_TEST(test_dump_small_buffer_keeps_newest_complete_lines);
    RUN_TEST(test_dump_small_buffer_lines_of_different_length);
    RUN_TEST(test_each_call_uses_the_critical_section);
    RUN_TEST(test_dump_cap_zero);
    RUN_TEST(test_parallel_logging_and_dumping);
    RUN_TEST(test_v1515_dump_never_mixes_lines);
    return UNITY_END();
}
