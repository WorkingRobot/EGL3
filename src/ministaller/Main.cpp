#include "Instance.h"

using namespace EGL3::Installer;

void operator delete(void*, size_t)
{
    
}

int Main() {
    {
        Instance::Initialize();
        Instance Inst;
    }

    ExitProcess(0);
    return 0;
}
