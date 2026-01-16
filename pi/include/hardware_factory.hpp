#ifndef HARDWARE_FACTORY_HPP
#define HARDWARE_FACTORY_HPP

#include "i_sensor_interface.hpp"
#include "i_pump_interface.hpp"
#include "simulated_hardware.hpp"
#include "real_hardware.hpp"
#include <memory>
#include <tuple>

struct HardwareBundle {
    std::shared_ptr<ISensorInterface> sensor;
    std::shared_ptr<IPumpInterface> pump;
    // Helper to keep the concrete object alive if shared pointers are just interfaces
    std::shared_ptr<void> hardwareInstance; 
};

class HardwareFactory {
public:
    static HardwareBundle createHardware(bool useSimulator) {
        HardwareBundle bundle;
        
        if (useSimulator) {
            auto sim = std::make_shared<SimulatedHardware>();
            bundle.hardwareInstance = sim;
            bundle.sensor = sim;
            bundle.pump = sim;
        } else {
            auto real = std::make_shared<RealHardware>();
            bundle.hardwareInstance = real;
            bundle.sensor = real;
            bundle.pump = real;
        }
        
        return bundle;
    }
};

#endif // HARDWARE_FACTORY_HPP
