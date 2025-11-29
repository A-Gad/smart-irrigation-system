// tests/unit/test_irrigation_logic.cpp
#include <gtest/gtest.h>
#include "irrigation_logic.hpp"

// Test Suite: Sensor Reading Validation
class SensorValidationTest : public ::testing::Test {};

TEST_F(SensorValidationTest, ValidReadingsAccepted) {
    EXPECT_TRUE(IrrigarionLogic::isReadingValid(0.0));
    EXPECT_TRUE(IrrigarionLogic::isReadingValid(50.0));
    EXPECT_TRUE(IrrigarionLogic::isReadingValid(100.0));
}

TEST_F(SensorValidationTest, InvalidReadingsRejected) {
    EXPECT_FALSE(IrrigarionLogic::isReadingValid(-10.0));
    EXPECT_FALSE(IrrigarionLogic::isReadingValid(110.0));
    EXPECT_FALSE(IrrigarionLogic::isReadingValid(-5.1));
    EXPECT_FALSE(IrrigarionLogic::isReadingValid(105.1));
}

TEST_F(SensorValidationTest, BoundaryValues) {
    EXPECT_TRUE(IrrigarionLogic::isReadingValid(-5.0));   // Lower boundary
    EXPECT_TRUE(IrrigarionLogic::isReadingValid(105.0));  // Upper boundary
}

// Test Suite: Filtered Moisture Calculation
class FilteredMoistureTest : public ::testing::Test {
protected:
    std::deque<sensorReading> readings;
    
    void addReading(double moisture, int secondsAgo = 0) {
        auto timestamp = std::chrono::steady_clock::now() - 
                        std::chrono::seconds(secondsAgo);
        readings.push_back({moisture, timestamp, true});
    }
};

TEST_F(FilteredMoistureTest, EmptyReadingsReturnsZero) {
    EXPECT_DOUBLE_EQ(IrrigarionLogic::getFilteredMoisture(readings), 0.0);
}

TEST_F(FilteredMoistureTest, SingleReadingReturnsValue) {
    addReading(45.0);
    EXPECT_DOUBLE_EQ(IrrigarionLogic::getFilteredMoisture(readings), 45.0);
}

TEST_F(FilteredMoistureTest, AveragesLastFiveReadings) {
    // Add 7 readings
    addReading(20.0, 70);
    addReading(25.0, 60);
    addReading(30.0, 50);  // Start of last 5
    addReading(35.0, 40);
    addReading(40.0, 30);
    addReading(45.0, 20);
    addReading(50.0, 10);
    
    // Should average last 5: (30+35+40+45+50)/5 = 40

    double expected = (30.0 + 35.0 + 40.0 + 45.0 + 50.0) / 5.0;
    EXPECT_DOUBLE_EQ(IrrigarionLogic::getFilteredMoisture(readings), expected);
}

TEST_F(FilteredMoistureTest, LessThanFiveReadingsUsesAll) {
    addReading(20.0, 30);
    addReading(30.0, 20);
    addReading(40.0, 10);
    
    double expected = (20.0 + 30.0 + 40.0) / 3.0;
    EXPECT_DOUBLE_EQ(IrrigarionLogic::getFilteredMoisture(readings), expected);
}

// Test Suite: Moisture Change Rate
class MoistureChangeRateTest : public ::testing::Test {
protected:
    std::deque<sensorReading> readings;
    
    void addReading(double moisture, int minutesAgo) {
        auto timestamp = std::chrono::steady_clock::now() - 
                        std::chrono::minutes(minutesAgo);
        readings.push_back({moisture, timestamp, true});
    }
};

TEST_F(MoistureChangeRateTest, InsufficientDataReturnsNullopt) {
    EXPECT_FALSE(IrrigarionLogic::getMoistuerChangeRate(readings).has_value());
    
    addReading(50.0, 0);
    EXPECT_FALSE(IrrigarionLogic::getMoistuerChangeRate(readings).has_value());
}

TEST_F(MoistureChangeRateTest, CalculatesPositiveChangeRate) {
    addReading(30.0, 10);  // 10 minutes ago
    addReading(50.0, 0);   // Now
    
    // Change: 50-30 = 20% over 10 minutes = 2% per minute
    auto rate = IrrigarionLogic::getMoistuerChangeRate(readings);
    ASSERT_TRUE(rate.has_value());
    EXPECT_DOUBLE_EQ(rate.value(), 2.0);
}

TEST_F(MoistureChangeRateTest, CalculatesNegativeChangeRate) {
    addReading(60.0, 20);  // 20 minutes ago
    addReading(40.0, 0);   // Now
    
    // Change: 40-60 = -20% over 20 minutes = -1% per minute
    auto rate = IrrigarionLogic::getMoistuerChangeRate(readings);
    ASSERT_TRUE(rate.has_value());
    EXPECT_DOUBLE_EQ(rate.value(), -1.0);
}

TEST_F(MoistureChangeRateTest, InvalidReadingsReturnNullopt) {
    readings.push_back({30.0, std::chrono::steady_clock::now(), false});  // Invalid
    readings.push_back({50.0, std::chrono::steady_clock::now(), true});
    
    EXPECT_FALSE(IrrigarionLogic::getMoistuerChangeRate(readings).has_value());
}

// Test Suite: Watering Decision Logic
class WateringDecisionTest : public ::testing::Test {};

TEST_F(WateringDecisionTest, ShouldStartWateringWhenAllConditionsMet) {
    bool result = IrrigarionLogic::shouldStartWatering(
        25.0,                          // filteredMoisture (below threshold)
        30.0,                          // threshold
        3,                             // consecutiveLowReadings (enough)
        std::chrono::minutes(60),      // timeSinceLastWatering
        30                             // minIntervalMinutes
    );
    
    EXPECT_TRUE(result);
}

TEST_F(WateringDecisionTest, ShouldNotStartWhenMoistureAboveThreshold) {
    bool result = IrrigarionLogic::shouldStartWatering(
        35.0,                          // Above threshold
        30.0,
        3,
        std::chrono::minutes(60),
        30
    );
    
    EXPECT_FALSE(result);
}

TEST_F(WateringDecisionTest, ShouldNotStartWithInsufficientConsecutiveReadings) {
    bool result = IrrigarionLogic::shouldStartWatering(
        25.0,
        30.0,
        2,                             // Only 2 readings (need 3)
        std::chrono::minutes(60),
        30
    );
    
    EXPECT_FALSE(result);
}

TEST_F(WateringDecisionTest, ShouldNotStartIfIntervalTooShort) {
    bool result = IrrigarionLogic::shouldStartWatering(
        25.0,
        30.0,
        3,
        std::chrono::minutes(20),      // Too soon (need 30 min)
        30
    );
    
    EXPECT_FALSE(result);
}

TEST_F(WateringDecisionTest, EdgeCaseExactThreshold) {
    bool result = IrrigarionLogic::shouldStartWatering(
        30.0,                          // Exactly at threshold
        30.0,
        3,
        std::chrono::minutes(60),
        30
    );
    
    EXPECT_FALSE(result);  // >= threshold means don't water
}

// Test Suite: Stop Watering Logic
class StopWateringTest : public ::testing::Test {};

TEST_F(StopWateringTest, StopsWhenTargetReached) {
    bool result = IrrigarionLogic::shouldStopWatering(
        70.0,                          // Reached target
        70.0,
        std::chrono::seconds(60),
        300,
        std::nullopt
    );
    
    EXPECT_TRUE(result);
}

TEST_F(StopWateringTest, StopsWhenMaxTimeExceeded) {
    bool result = IrrigarionLogic::shouldStopWatering(
        50.0,                          // Not at target
        70.0,
        std::chrono::seconds(301),     // Exceeded max time
        300,
        std::nullopt
    );
    
    EXPECT_TRUE(result);
}

TEST_F(StopWateringTest, StopsWhenMoistureNotIncreasing) {
    bool result = IrrigarionLogic::shouldStopWatering(
        50.0,
        70.0,
        std::chrono::seconds(20),      // Enough time elapsed
        300,
        0.3                            // Very low change rate (< 0.5)
    );
    
    EXPECT_TRUE(result);  // Possible pump failure
}

TEST_F(StopWateringTest, ContinuesWhenConditionsNormal) {
    bool result = IrrigarionLogic::shouldStopWatering(
        50.0,                          // Below target
        70.0,
        std::chrono::seconds(60),      // Within time limit
        300,
        1.5                            // Good change rate
    );
    
    EXPECT_FALSE(result);  // Keep watering
}

TEST_F(StopWateringTest, IgnoresLowChangeRateInEarlyPhase) {
    bool result = IrrigarionLogic::shouldStopWatering(
        50.0,
        70.0,
        std::chrono::seconds(5),       // Too early (< 10s)
        300,
        0.1                            // Low rate, but early
    );
    
    EXPECT_FALSE(result);  // Give it time
}