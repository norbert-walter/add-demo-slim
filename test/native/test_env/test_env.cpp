// Sprint 0a: proves that the native Unity test environment works at all.
// FROZEN – do not modify.
#include <unity.h>

void setUp() {}
void tearDown() {}

void test_environment_runs() { TEST_ASSERT_EQUAL_INT(4, 2 + 2); }

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_environment_runs);
    return UNITY_END();
}
