#ifndef REAL_HARDWARE_HPP
#define REAL_HARDWARE_HPP

#include "i_sensor_interface.hpp"
#include "i_pump_interface.hpp"

class RealHardware : public ISensorInterface, public IPumpInterface {
public:
    RealHardware() = default;
    ~RealHardware() override = default;

    // ISensorInterface implementation
    bool initialize() override;
    double getMoisture() override;
    double getTemp() override;
    double getHumid() override;
    bool isRainDetected() override;
    bool isHealthy() override;

    // IPumpInterface implementation
    void activate() override;
    void deactivate() override;
    bool isActive() override;

private:
    bool pumpState = false;
};

#endif // REAL_HARDWARE_HPP
