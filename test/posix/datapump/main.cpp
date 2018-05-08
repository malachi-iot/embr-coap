#include <iostream>

using namespace std;

int blocking_datapump_handler(volatile const bool& service_active);

int nonblocking_datapump_setup();
void nonblocking_datapump_loop(int);
int nonblocking_datapump_shutdown(int);

int main()
{
    cout << "Hello World!" << endl;

    int handle = nonblocking_datapump_setup();

    for(;;)
    {
        nonblocking_datapump_loop(handle);
    }

    nonblocking_datapump_shutdown(handle);

    return 0;
}
