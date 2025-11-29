#ifndef MOCK_INTERFACES_HPP
#define MOCK_INTERFACES_HPP

#include <gmock/gmock.h>
#include "i_sensor_interface.hpp"
#include "i_pump_interface.hpp"

// Mock Sensor Interface
class MockSensorInterface : public ISensorInterface
{
    public:
        virtual ~MockSensorInterface() = default;

        MOCK_METHOD(double, getMoisture, (), (override));
        MOCK_METHOD(double, getTemp, (), (override));
        MOCK_METHOD(double, getHumid, (), (override));
        MOCK_METHOD(bool, isRainDetected, (), (override));
        MOCK_METHOD(bool, isHealthy, (), (override));
        MOCK_METHOD(bool, initialize, (), (override));
};

// Mock Pump Interface
class MockPumpInterface : public IPumpInterface
{
    public:
        virtual ~MockPumpInterface() = default;

        MOCK_METHOD(void, activate, (), (override));
        MOCK_METHOD(void, deactivate, (), (override));
        MOCK_METHOD(bool, isActive, (), (override));
        MOCK_METHOD(bool, initialize, (), (override));
};

#endif