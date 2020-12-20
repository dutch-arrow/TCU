#include "sensors.h"
#include <unity.h>

rule_set_t rules[2];

void setUp(void) {
}

void tearDown(void) {
// clean stuff up here
}

void test_rule1(void) {
    check_sensor_rules(1, 1, 1, 1, &rules[0]);
    TEST_ASSERT_EQUAL(32, 25 + 7);
}


int main(int argc, char **argv) {
    UNITY_BEGIN();

    RUN_TEST(test_rule1);

    UNITY_END();
    return 0;
}
