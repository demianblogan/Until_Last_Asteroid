#include <SFML/Config.hpp>

#include "app/Application.h"

static_assert(SFML_VERSION_MAJOR == 3 && SFML_VERSION_MINOR == 1 && SFML_VERSION_PATCH == 0,
    "Until Last Asteroid requires SFML 3.1.0.");

int main()
{
    Application application;
    application.Run();

    return 0;
}
