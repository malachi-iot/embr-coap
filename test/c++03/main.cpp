#include <unit-test.h>

void setUp (void) {}
void tearDown (void) {}

int main()
{
    UNITY_BEGIN();
    test_decoder();
    test_encoder();
    test_header();
    UNITY_END();

    return 0;
}