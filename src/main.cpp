#include "app.hpp"

#include <cstdlib>

#ifdef _WIN32
#include <Windows.h>

int WINAPI wWinMain([[maybe_unused]] _In_ HINSTANCE hInstance,
                    [[maybe_unused]] _In_opt_ HINSTANCE hPrevInstance,
                    [[maybe_unused]] _In_ LPWSTR lpCmdLine,
                    [[maybe_unused]] _In_ int nShowCmd)
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

