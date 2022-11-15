#pragma once

#include "config-utils/shared/config-utils.hpp"


DECLARE_CONFIG(ModConfig,
    CONFIG_VALUE(LastCrash, std::string, "Last Crash", "");
    CONFIG_VALUE(ShowPopup, bool, "Show Popup", true);
    CONFIG_VALUE(Simple, bool, "Simple", true);

    CONFIG_INIT_FUNCTION(
        CONFIG_INIT_VALUE(LastCrash);
        CONFIG_INIT_VALUE(ShowPopup);
        CONFIG_INIT_VALUE(Simple);

    )
)