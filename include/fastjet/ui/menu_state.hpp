#pragma once

#include <cstdint>

namespace fastjet::ui {

/// @brief Context under which the tactical menu was summoned
enum class MenuContext : uint8_t {
    BOOT_DISPATCH = 0,    ///< Initial application startup dispatch & readiness center
    IN_FLIGHT_RECONFIG,   ///< Summoned mid-flight via 'M' key or HOTAS menu button
};

/// @brief Active operational tab in the tactical avionics menu system
enum class MenuTab : uint8_t {
    AIRFRAME_SELECT = 0,  ///< Airframe inspection, comparison, and active switch
    FLIGHT_ENVELOPE,      ///< Detailed aerodynamic polars, propulsion curves, limits
    HARDWARE_CALIBRATION, ///< Live HOTAS stick, throttle curves, detents, and deadzones
    SORTIE_DISPATCH,      ///< Pre-flight readiness, runway scramble, weather & fuel
    COUNT
};

} // namespace fastjet::ui
