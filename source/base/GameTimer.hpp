////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2023-2024. Matt Guerrette. All rights reserved.
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <algorithm>
#include <chrono>

using namespace std::chrono_literals;

class GameTimer
{
    static constexpr uint64_t TicksPerSecond = 10000000;

public:
    GameTimer();

    ~GameTimer() = default;

    [[nodiscard]] uint64_t ElapsedTicks() const noexcept;

    [[nodiscard]] double ElapsedSeconds() const noexcept;

    [[nodiscard]] uint64_t TotalTicks() const noexcept;

    [[nodiscard]] double TotalSeconds() const noexcept;

    [[nodiscard]] uint32_t FrameCount() const noexcept;

    [[nodiscard]] uint32_t FramesPerSecond() const noexcept;

    void SetFixedTimeStep(bool isFixedTimeStep) noexcept;

    void SetTargetElapsedTicks(uint64_t targetElapsed) noexcept;

    void SetTargetElapsedSeconds(double targetElapsed) noexcept;

    void ResetElapsedTime();

    /// @brief Updates the game timer and calls the provided update function.
    ///
    /// This function is responsible for calculating the time elapsed since the last update,
    /// updating the game timer's internal state, and calling the provided update function.
    /// It also keeps track of the frame count and frames per second.
    ///
    /// @tparam TUpdate The type of the update function.
    /// @param [in] update The update function to be called.
    template <typename TUpdate>
    void Tick(const TUpdate& update)
    {

        auto now = std::chrono::steady_clock::now();
        auto now_in_ns = std::chrono::time_point_cast<std::chrono::nanoseconds>(now);
        auto epoch = now_in_ns.time_since_epoch();
        auto value = std::chrono::duration_cast<std::chrono::nanoseconds>(epoch);

        const uint64_t currentTime = static_cast<uint64_t>(value.count());

        uint64_t delta = currentTime - QpcLastTime_;
        QpcLastTime_ = currentTime;
        QpcSecondCounter_ += delta;

        delta = std::clamp(delta, static_cast<uint64_t>(0), QpcMaxDelta_);
        delta *= TicksPerSecond;
        delta /= static_cast<uint64_t>(QpcFrequency_);

        const uint32_t lastFrameCount = FrameCount_;
        if (IsFixedTimeStep_)
        {
            if (static_cast<uint64_t>(std::abs(static_cast<int64_t>(delta - TargetElapsedTicks_))) <
                TicksPerSecond / 4000)
            {
                delta = TargetElapsedTicks_;
            }

            LeftOverTicks_ += delta;
            while (LeftOverTicks_ >= TargetElapsedTicks_)
            {
                ElapsedTicks_ = TargetElapsedTicks_;
                TotalTicks_ += TargetElapsedTicks_;
                LeftOverTicks_ -= TargetElapsedTicks_;
                FrameCount_++;

                update();
            }
        }
        else
        {
            ElapsedTicks_ = delta;
            TotalTicks_ += delta;
            LeftOverTicks_ = 0;
            FrameCount_++;

            update();
        }

        if (FrameCount_ != lastFrameCount)
        {
            FramesThisSecond_++;
        }

        if (QpcSecondCounter_ >= QpcFrequency_)
        {
            FramesPerSecond_ = FramesThisSecond_;
            FramesThisSecond_ = 0;
            QpcSecondCounter_ %= QpcFrequency_;
        }
    }

    static constexpr double TicksToSeconds(const uint64_t ticks) noexcept
    {
        return static_cast<double>(ticks) / TicksPerSecond;
    }

    static constexpr uint64_t SecondsToTicks(const double seconds) noexcept
    {
        return static_cast<uint64_t>(seconds * TicksPerSecond);
    }

private:
    uint64_t QpcFrequency_;
    uint64_t QpcLastTime_;
    uint64_t QpcMaxDelta_;
    uint64_t QpcSecondCounter_;
    uint64_t ElapsedTicks_;
    uint64_t TotalTicks_;
    uint64_t LeftOverTicks_;
    uint32_t FrameCount_;
    uint32_t FramesPerSecond_;
    uint32_t FramesThisSecond_;
    bool     IsFixedTimeStep_;
    uint64_t TargetElapsedTicks_;
};
