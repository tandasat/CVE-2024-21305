#include <Windows.h>

int main()
{
    // Make sure beep!BeepOpen is executed on CPU0 there TLB was explicitly
    // invalidated.
    SetThreadAffinityMask(GetCurrentThread(), 1);

    // Execute beep!BeepOpen. Will die.
    Beep(1000, 1000);
}
