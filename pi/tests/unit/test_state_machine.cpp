// tests/unit/test_state_machine.cpp
#include <gtest/gtest.h>
#include "test_fixtures.hpp"

using ::testing::Return;
using ::testing::_;
using ::testing::AtLeast;

// Test Suite: Initialization
class StateMachineInitTest : public StateMachineTestFixture {};

TEST_F(StateMachineInitTest, StartsInIdleState) {
    auto sm = createStateMachine();
    EXPECT_EQ(sm->getCurrentState(), SystemState::IDLE);
}

TEST_F(StateMachineInitTest, InitializesWithSensorReading) {
    EXPECT_CALL(mockSensor, getMoisture())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(45.0));
    
    auto sm = createStateMachine();
    // Verify initialization happened without crash
}

TEST_F(StateMachineInitTest, PumpIsOffOnStartup) {
    EXPECT_CALL(mockPump, isActive())
        .WillOnce(Return(false));
    
    auto sm = createStateMachine();
    sm->update();
    // No pump start should be called
}

// Test Suite: Command Processing
class CommandProcessingTest : public StateMachineTestFixture {};

TEST_F(CommandProcessingTest, StartAutoTransitionsToMonitoring) {
    auto sm = createStateMachine();
    
    sm->sendCommnd(Command::START_AUTO);
    sm->update();
    
    EXPECT_EQ(sm->getCurrentState(), SystemState::MONITORING);
}

TEST_F(CommandProcessingTest, EnableManualTransitionsToManual) {
    auto sm = createStateMachine();
    
    sm->sendCommnd(Command::ENABLE_MANUAL);
    sm->update();
    
    EXPECT_EQ(sm->getCurrentState(), SystemState::MANUAL);
}

TEST_F(CommandProcessingTest, EmergencyStopGoesToError) {
    auto sm = createStateMachine();
    sm->sendCommnd(Command::START_AUTO);
    sm->update();
    
    EXPECT_CALL(mockPump, deactivate()).Times(AtLeast(1));
    
    sm->sendCommnd(Command::EMERGENCY_STOP);
    sm->update();
    
    EXPECT_EQ(sm->getCurrentState(), SystemState::ERROR);
}

TEST_F(CommandProcessingTest, MultipleCommandsProcessedInOrder) {
    auto sm = createStateMachine();
    
    // Queue multiple commands
    sm->sendCommnd(Command::START_AUTO);
    sm->sendCommnd(Command::ENABLE_MANUAL);
    
    sm->update();  // Should process START_AUTO
    EXPECT_EQ(sm->getCurrentState(), SystemState::MONITORING);
    
    sm->update();  // Should process ENABLE_MANUAL
    EXPECT_EQ(sm->getCurrentState(), SystemState::MANUAL);
}

// Test Suite: IDLE State Behavior
class IdleStateTest : public StateMachineTestFixture {};

TEST_F(IdleStateTest, DetectsSensorFailure) {
    EXPECT_CALL(mockSensor, isHealthy())
        .WillOnce(Return(false))
        .WillOnce(Return(false))
        .WillOnce(Return(false));
    
    auto sm = createStateMachine();
    
    sm->update();  // First failure
    EXPECT_EQ(sm->getCurrentState(), SystemState::IDLE);
    
    sm->update();  // Second failure
    EXPECT_EQ(sm->getCurrentState(), SystemState::IDLE);
    
    sm->update();  // Third failure - should go to ERROR
    EXPECT_EQ(sm->getCurrentState(), SystemState::ERROR);
}

TEST_F(IdleStateTest, EnsuresPumpIsStopped) {
    EXPECT_CALL(mockPump, isActive()).WillOnce(Return(true));
    EXPECT_CALL(mockPump, deactivate()).Times(1);
    
    auto sm = createStateMachine();
    sm->update();
}

TEST_F(IdleStateTest, StaysIdleWhenRaining) {
    EXPECT_CALL(mockSensor, isRainDetected()).WillRepeatedly(Return(true));
    
    auto sm = createStateMachine();
    
    // Wait for auto-start timer
    advanceTime(35);
    sm->update();
    
    EXPECT_EQ(sm->getCurrentState(), SystemState::IDLE);  // Still idle
}

// Test Suite: MONITORING State
class MonitoringStateTest : public StateMachineTestFixture {};

TEST_F(MonitoringStateTest, TransitionsToWateringWhenDry) {
    auto sm = createStateMachine();
    sm->sendCommnd(Command::START_AUTO);
    sm->update();
    
    // Simulate low moisture readings
    EXPECT_CALL(mockSensor, getMoisture())
        .WillRepeatedly(Return(20.0));  // Below 30% threshold
    
    // Need 3 consecutive low readings
    sm->update();
    sm->update();
    sm->update();
    
    EXPECT_EQ(sm->getCurrentState(), SystemState::WATERING);
}

TEST_F(MonitoringStateTest, DoesNotWaterIfIntervalTooShort) {
    config.minWateringIntervalMinutes = 60;
    auto sm = createStateMachine();
    sm->sendCommnd(Command::START_AUTO);
    sm->update();
    
    EXPECT_CALL(mockSensor, getMoisture())
        .WillRepeatedly(Return(20.0));
    
    // Multiple updates with low moisture
    for (int i = 0; i < 5; ++i) {
        sm->update();
    }
    
    // Should stay in MONITORING (interval not met)
    EXPECT_EQ(sm->getCurrentState(), SystemState::MONITORING);
}

TEST_F(MonitoringStateTest, ResetsLowReadingCounterWhenMoistureNormal) {
    auto sm = createStateMachine();
    sm->sendCommnd(Command::START_AUTO);
    sm->update();
    
    // Two low readings
    EXPECT_CALL(mockSensor, getMoisture())
        .WillOnce(Return(20.0))
        .WillOnce(Return(20.0))
        .WillOnce(Return(50.0));  // Normal reading
    
    sm->update();  // Low
    sm->update();  // Low
    sm->update();  // Normal - resets counter
    
    // Now need 3 more low readings to water
    EXPECT_CALL(mockSensor, getMoisture())
        .WillRepeatedly(Return(20.0));
    
    sm->update();
    sm->update();
    EXPECT_EQ(sm->getCurrentState(), SystemState::MONITORING);  // Only 2 readings
    
    sm->update();  // 3rd reading
    EXPECT_EQ(sm->getCurrentState(), SystemState::WATERING);
}

// Test Suite: WATERING State
class WateringStateTest : public StateMachineTestFixture {};

TEST_F(WateringStateTest, StartsPumpOnEntry) {
    auto sm = createStateMachine();
    sm->sendCommnd(Command::START_AUTO);
    sm->update();
    
    // Force transition to watering
    EXPECT_CALL(mockSensor, getMoisture())
        .WillRepeatedly(Return(20.0));
    
    for (int i = 0; i < 3; ++i) sm->update();
    
    EXPECT_CALL(mockPump, isActive()).WillRepeatedly(Return(false));
    EXPECT_CALL(mockPump, activate()).Times(AtLeast(1));
    
    sm->update();  // First update in WATERING state
}

TEST_F(WateringStateTest, StopsWhenTargetReached) {
    auto sm = createStateMachine();
    
    // Get to watering state
    sm->sendCommnd(Command::START_AUTO);
    EXPECT_CALL(mockSensor, getMoisture())
        .WillOnce(Return(20.0))
        .WillOnce(Return(20.0))
        .WillOnce(Return(20.0))
        .WillRepeatedly(Return(70.0));  // Target reached
    
    sm->update();
    sm->update();
    sm->update();
    sm->update();  // Transition to WATERING
    
    EXPECT_CALL(mockPump, isActive()).WillRepeatedly(Return(true));
    EXPECT_CALL(mockPump, deactivate()).Times(1);
    
    sm->update();  // Check target, should stop
    
    EXPECT_EQ(sm->getCurrentState(), SystemState::WAITING);
}

TEST_F(WateringStateTest, StopsOnTimeout) {
    config.maxWateringSeconds = 5;  // Short timeout for testing
    auto sm = createStateMachine();
    
    // Get to watering state
    sm->sendCommnd(Command::START_AUTO);
    EXPECT_CALL(mockSensor, getMoisture())
        .WillRepeatedly(Return(20.0));  // Never reaches target
    
    for (int i = 0; i < 3; ++i) sm->update();
    
    EXPECT_CALL(mockPump, isActive()).WillRepeatedly(Return(true));
    EXPECT_CALL(mockPump, deactivate()).Times(1);
    
    // Wait for timeout
    advanceTime(6);
    sm->update();
    
    EXPECT_EQ(sm->getCurrentState(), SystemState::ERROR);  // Timeout error
}

// Test Suite: Thread Safety
class ThreadSafetyTest : public StateMachineTestFixture {};

TEST_F(ThreadSafetyTest, ConcurrentCommandSubmission) {
    auto sm = createStateMachine();
    
    std::vector<std::thread> threads;
    
    // Multiple threads sending commands
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&sm]() {
            for (int j = 0; j < 100; ++j) {
                sm->sendCommnd(Command::START_AUTO);
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    // No crash = success
    sm->update();
}

TEST_F(ThreadSafetyTest, ConcurrentConfigUpdate) {
    auto sm = createStateMachine();
    
    std::thread updater([&sm, this]() {
        for (int i = 0; i < 100; ++i) {
            IrrigationConfig newConfig = config;
            newConfig.lowMoistureThreshold = 20.0 + i;
            sm->updateConfig(newConfig);
        }
    });
    
    // Main thread reading config
    for (int i = 0; i < 100; ++i) {
        auto cfg = sm->getConfig();
        EXPECT_GE(cfg.lowMoistureThreshold, 20.0);
    }
    
    updater.join();
}