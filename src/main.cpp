#include <SFML/Config.hpp>

#include "app/Application.h"

// On laptops with hybrid graphics (integrated + discrete GPU), Windows lets
// the NVIDIA/AMD driver pick which GPU runs each process, based on a
// built-in database of known games/publishers. A small indie exe not in
// that database can silently get routed to the weak integrated GPU even
// when a discrete one is sitting idle. Exporting these two symbols is the
// NVIDIA/AMD-documented way to force the discrete GPU for this process
// instead, bypassing that heuristic. On desktops with a single GPU (or no
// hybrid setup at all) this has no effect. DWORD is just `unsigned long`
// here so this doesn't need to pull in <Windows.h> for one typedef.
extern "C"
{
    __declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

static_assert(
    SFML_VERSION_MAJOR == 3 && 
    SFML_VERSION_MINOR == 1 && 
    SFML_VERSION_PATCH == 0,
    "Until Last Asteroid requires SFML 3.1.0."
    );

int main()
{
    Application application;
    application.Run();

    return 0;
}