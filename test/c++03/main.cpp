#include <unit-test.h>

void setUp (void) {}
void tearDown (void) {}

int main()
{
    UNITY_BEGIN();
    test_decoder();
    test_encoder();
    test_header();
    test_retry();
    test_uri();
    UNITY_END();

    return 0;
}