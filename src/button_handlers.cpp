#include "button_handlers.hpp"

#include "network_utils.hpp"

void setupAccessPointOnRelease(const int time)
{
    if (time > 3000)
    {
        commitEEPROMFlag(0, WIFI_CONFIGURED_FLAG);
        startSetupAp();
        return;
    }
}
