#include "app.h"

#include <cstdlib>

#ifdef _WIN32
#include <Windows.h>

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int)
#else
int main()
#endif
{
    App app;
    if (!app.init())
    {
        return EXIT_FAILURE;
    }

    app.run();
    app.cleanup();

    return EXIT_SUCCESS;
}
