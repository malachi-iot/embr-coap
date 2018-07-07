void coap_daemon(void* param);

void test_task(void* param)
{
    coap_daemon(param);
}