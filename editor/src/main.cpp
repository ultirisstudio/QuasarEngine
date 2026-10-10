#include <quasar/Version.h>

#include <cstdio>

int main()
{
    std::printf("QuasarEngine Editor %s\n", Quasar::GetVersionString());
    return 0;
}
