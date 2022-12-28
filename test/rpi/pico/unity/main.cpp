#include <unity.h>

#include <test/support.h>

int main()
{
    test::v1::init();

    for(;;) test::v1::cyw43_poll();

    return 0;
}
