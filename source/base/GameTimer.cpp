////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2023-2024. Matt Guerrette. All rights reserved.
////////////////////////////////////////////////////////////////////////////////

#include "GameTimer.hpp"

#define TIMER_FREQUENCY 1000000000LL

GameTimer::GameTimer()
    : ElapsedTicks_(0), TotalTicks_(0), LeftOverTicks_(0), FrameCount_(0), FramesPerSecond_(0),
      FramesThisSecond_(0), QpcSecondCounter_(0), IsFixedTimeStep_(false), TargetElapsedTicks_(0)
{
    QpcFrequency_ = TIMER_FREQUENCY;

    auto now = std::chrono::steady_clock::now();
    auto nowNs = std::chrono::time_point_cast<std::chrono::nanoseconds>(now);
    auto epoch = nowNs.time_since_epoch();
    auto value = std::chrono::duration_cast<std::chrono::nanoseconds>(epoch);
    QpcLastTime_ = static_cast<uint64_t>(value.count());

    // Max delta 1/10th of a second
    QpcMaxDelta_ = QpcFrequency_ / 10;
}

uint64_t GameTimer::ElapsedTicks() const noexcept
{
    return ElapsedTicks_;
}

double GameTimer::ElapsedSeconds() const noexcept
{
    return TicksToSeconds(ElapsedTicks_);
}

uint64_t GameTimer::TotalTicks() const noexcept
{
    return TotalTicks_;
}

double GameTimer::TotalSeconds() const noexcept
{
    return TicksToSeconds(TotalTicks_);
}

uint32_t GameTimer::FrameCount() const noexcept
{
    return FrameCount_;
}

uint32_t GameTimer::FramesPerSecond() const noexcept
{
    return FramesPerSecond_;
}

void GameTimer::SetFixedTimeStep(const bool isFixedTimeStep) noexcept
{
    IsFixedTimeStep_ = isFixedTimeStep;
}

void GameTimer::SetTargetElapsedTicks(const uint64_t targetElapsed) noexcept
{
    TargetElapsedTicks_ = targetElapsed;
}

void GameTimer::SetTargetElapsedSeconds(const double targetElapsed) noexcept
{
    TargetElapsedTicks_ = SecondsToTicks(targetElapsed);
}

void GameTimer::ResetElapsedTime()
{
    auto now = std::chrono::steady_clock::now();
    auto nowNs = std::chrono::time_point_cast<std::chrono::nanoseconds>(now);
    auto epoch = nowNs.time_since_epoch();
    auto value = std::chrono::duration_cast<std::chrono::nanoseconds>(epoch);
    QpcLastTime_ = static_cast<uint64_t>(value.count());

    LeftOverTicks_ = 0;
    FramesPerSecond_ = 0;
    FramesThisSecond_ = 0;
    QpcSecondCounter_ = 0;
    TotalTicks_ = 0;
}
