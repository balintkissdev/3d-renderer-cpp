#include "app.h"

#include <cstdlib>

int main()
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
