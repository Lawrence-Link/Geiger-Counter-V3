#pragma once

#include <stddef.h>
#include <stdint.h>

using ChartSample = int32_t;

template <size_t Capacity>
class StaticChartSeries;

class ChartSeries {
public:
    /** @brief Copy construction is disabled because storage is externally owned. */
    ChartSeries(const ChartSeries&) = delete;
    /** @brief Copy assignment is disabled because storage is externally owned. */
    ChartSeries& operator=(const ChartSeries&) = delete;
    /** @brief Move construction is disabled because storage is externally owned. */
    ChartSeries(ChartSeries&&) = delete;
    /** @brief Move assignment is disabled because storage is externally owned. */
    ChartSeries& operator=(ChartSeries&&) = delete;

    /** @brief Appends a sample, overwriting the oldest sample when full. */
    void add(ChartSample sample);
    /** @brief Removes current and historical samples and advances the revision. */
    void clear();

    /** @return Number of samples the current window can hold. */
    size_t capacity() const { return capacity_; }
    /** @return Number of samples currently in the window. */
    size_t size() const { return count_; }
    /** @return Whether the current window contains no samples. */
    bool empty() const { return count_ == 0U; }
    /** @return Monotonic revision incremented after add and clear. */
    uint32_t revision() const { return revision_; }

    /** @return Sample offset positions back from newest, or zero when out of range. */
    ChartSample sampleFromNewest(size_t offset) const;

    /** @return Minimum value in the current window, or zero when empty. */
    ChartSample minimum() const;
    /** @return Maximum value in the current window, or zero when empty. */
    ChartSample maximum() const;
    /** @return Truncated average of the current window, or zero when empty. */
    ChartSample average() const;

    /** @return Minimum value seen since the last clear, or zero when empty. */
    ChartSample historyMinimum() const;
    /** @return Maximum value seen since the last clear, or zero when empty. */
    ChartSample historyMaximum() const;
    /** @return Truncated average of all values seen since the last clear. */
    ChartSample historyAverage() const;
    /** @return Number of samples seen since the last clear. */
    uint32_t historyCount() const { return historyCount_; }

private:
    template <size_t Capacity>
    friend class StaticChartSeries;

    /** @brief Creates a view over fixed storage supplied by StaticChartSeries. */
    ChartSeries(ChartSample* storage, size_t capacity)
        : storage_(storage), capacity_(capacity) {}

    /** @brief Recomputes current-window minimum and maximum after an overwrite. */
    void recalculateExtremes();

    ChartSample* storage_ = nullptr;
    size_t capacity_ = 0U;
    size_t writeIndex_ = 0U;
    size_t count_ = 0U;
    int64_t sum_ = 0;
    ChartSample minimum_ = 0;
    ChartSample maximum_ = 0;

    int64_t historySum_ = 0;
    ChartSample historyMinimum_ = 0;
    ChartSample historyMaximum_ = 0;
    uint32_t historyCount_ = 0U;
    uint32_t revision_ = 0U;
};

template <size_t Capacity>
class StaticChartSeries : public ChartSeries {
    static_assert(Capacity > 0U, "Chart series capacity must be greater than zero");

public:
    static constexpr size_t staticCapacity = Capacity;

    /** @brief Creates an empty chart series with compile-time fixed capacity. */
    StaticChartSeries() : ChartSeries(storage_, Capacity) {}

private:
    ChartSample storage_[Capacity];
};

template <uint16_t Width, uint16_t Height>
struct ChartExpandSize {
    static_assert(Width > 0U, "Expanded chart width must be greater than zero");
    static_assert(Height > 0U, "Expanded chart height must be greater than zero");

    static constexpr uint16_t width = Width;
    static constexpr uint16_t height = Height;
};
