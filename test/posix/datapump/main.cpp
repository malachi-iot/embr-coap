#include <iostream>

using namespace std;

int blocking_datapump_handler(volatile const bool& service_active);

int main()
{
    cout << "Hello World!" << endl;

    bool service_active = true;

    blocking_datapump_handler(service_active);

    return 0;
}
