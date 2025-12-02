// tests/integration/test_watering_cycle.cpp
#include <gtest/gtest.h>
#include "test_fixtures.hpp"

using ::testing::Return;
using ::testing::_;

class WateringCycleIntegrationTest : public StateMachineTestFixture {};

TEST_F(WateringCycleIntegrationTest, CompleteWateringCycleSucceeds) {
    // Scenario: Dry soil → water → reach target → wait → monitor again
    config.minWateringIntervalMinutes = 0;  // Critical!
    config.lowMoistureThreshold = 30.0;
    config.highMoistureThreshold = 70.0;
    auto sm = createStateMachine();
    
    // Phase 1: Start monitoring (setup expectations first)
    // Phase 2: Detect low moisture (3 consecutive readings)
    EXPECT_CALL(mockSensor, getMoisture())
        .WillOnce(Return(25.0)) // START_AUTO
        .WillOnce(Return(25.0))
        .WillOnce(Return(24.0))
        .WillOnce(Return(23.0))
        .WillOnce(Return(30.0))  // Start rising
        .WillOnce(Return(45.0))
        .WillOnce(Return(60.0))
        .WillOnce(Return(70.0))  // Target reached
        .WillRepeatedly(Return(70.0));
    
    EXPECT_CALL(mockPump, isActive())   
        .WillOnce(Return(false))        // Not running initially
        .WillRepeatedly(Return(true));
    EXPECT_CALL(mockPump, activate()).Times(1);
    EXPECT_CALL(mockPump, deactivate()).Times(1);
    
    sm->sendCommnd(Command::START_AUTO);
    sm->update();
    EXPECT_EQ(sm->getCurrentState(), SystemState::MONITORING);
    
    // Get 3 low readings
    sm->update();
    sm->update();
    sm->update();
    sm->update();
    // Should transition to WATERING
    EXPECT_EQ(sm->getCurrentState(), SystemState::WATERING);
    
    // Phase 3: Watering continues until target reached
    EXPECT_CALL(mockPump, isActive())
        .WillRepeatedly(Return(true));
    
    sm->update();  // 30%
    EXPECT_EQ(sm->getCurrentState(), SystemState::WATERING);
    
    sm->update();  // 45%
    EXPECT_EQ(sm->getCurrentState(), SystemState::WATERING);
    
    sm->update();  // 60%
    EXPECT_EQ(sm->getCurrentState(), SystemState::WATERING);
    
    sm->update();  // 60%
    EXPECT_EQ(sm->getCurrentState(), SystemState::WATERING);
    
    // Update multiple times to flush the moving average buffer
    for(int i=0; i<5; i++) sm->update();
    
    EXPECT_EQ(sm->getCurrentState(), SystemState::WAITING);
    
    // Phase 4: Wait period
    config.waitMinutes = 1;
}  // Short wait