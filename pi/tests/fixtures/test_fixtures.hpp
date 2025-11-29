// tests/fixtures/test_fixtures.hpp
#ifndef TEST_FIXTURES_HPP
#define TEST_FIXTURES_HPP

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "mock_interfaces.hpp"
#include "state_machine.hpp"
#include <thread>
// Base fixture for state machine tests
class StateMachineTestFixture : public ::testing::Test {
protected:
    void SetUp() override {
        // Default configuration
        config.zoneName = "TestZone";
        config.lowMoistureThreshold = 30.0;
        config.highMoistureThreshold = 70.0;
        config.minWateringIntervalMinutes = 30;
        config.maxWateringSeconds = 300;
        config.waitMinutes= 10;
        
        // Set default mock behaviors
        setupDefaultMockBehavior();
    }
    
    void TearDown() override {
        // Cleanup if needed
    }
    
    void setupDefaultMockBehavior() {
        using ::testing::Return;
        
        // Healthy sensor by default
        ON_CALL(mockSensor, isHealthy()).WillByDefault(Return(true));
        ON_CALL(mockSensor, getMoisture()).WillByDefault(Return(50.0));
        ON_CALL(mockSensor, getTemp()).WillByDefault(Return(25.0));
        ON_CALL(mockSensor, getHumid()).WillByDefault(Return(60.0));
        ON_CALL(mockSensor, isRainDetected()).WillByDefault(Return(false));
        
        // Pump off by default
        ON_CALL(mockPump, isActive()).WillByDefault(Return(false));
    }
    
    // Helper method to create state machine
    std::unique_ptr<StateMachine> createStateMachine() {
        return std::make_unique<StateMachine>(&mockSensor, &mockPump, config);
    }
    
    // Helper to advance time (simulate time passing)
    void advanceTime(int seconds) {
        for (int i = 0; i < seconds; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    
    // Test doubles
    MockSensorInterface mockSensor;
    MockPumpInterface mockPump;
    IrrigationConfig config;
};

#endif // TEST_FIXTURES_HPP