#include "widgets/ChartSeries.h"

#include <etl/algorithm.h>
#include <etl/limits.h>

void ChartSeries::add(ChartSample sample) {
    const bool replacing = count_ == capacity_;
    const ChartSample replaced = replacing ? storage_[writeIndex_] : 0;

    storage_[writeIndex_] = sample;
    writeIndex_ = (writeIndex_ + 1U) % capacity_;
    if (!replacing) ++count_;

    historyMinimum_ = historyCount_ == 0U
        ? sample
        : etl::min(historyMinimum_, sample);
    historyMaximum_ = historyCount_ == 0U
        ? sample
        : etl::max(historyMaximum_, sample);
    historySum_ += sample;
    ++historyCount_;

    if (!replacing) {
        minimum_ = count_ == 1U ? sample : etl::min(minimum_, sample);
        maximum_ = count_ == 1U ? sample : etl::max(maximum_, sample);
        sum_ += sample;
    } else {
        sum_ = sum_ - replaced + sample;
        if (replaced == minimum_ || replaced == maximum_) {
            recalculateExtremes();
        } else {
            minimum_ = etl::min(minimum_, sample);
            maximum_ = etl::max(maximum_, sample);
        }
    }

    ++revision_;
}

void ChartSeries::clear() {
    writeIndex_ = 0U;
    count_ = 0U;
    sum_ = 0;
    minimum_ = 0;
    maximum_ = 0;
    historySum_ = 0;
    historyMinimum_ = 0;
    historyMaximum_ = 0;
    historyCount_ = 0U;
    ++revision_;
}

ChartSample ChartSeries::sampleFromNewest(size_t offset) const {
    if (offset >= count_) return 0;
    const size_t index = (writeIndex_ + capacity_ - 1U - offset) % capacity_;
    return storage_[index];
}

ChartSample ChartSeries::minimum() const {
    return empty() ? 0 : minimum_;
}

ChartSample ChartSeries::maximum() const {
    return empty() ? 0 : maximum_;
}

ChartSample ChartSeries::average() const {
    return empty() ? 0 : static_cast<ChartSample>(sum_ / count_);
}

ChartSample ChartSeries::historyMinimum() const {
    return historyCount_ == 0U ? 0 : historyMinimum_;
}

ChartSample ChartSeries::historyMaximum() const {
    return historyCount_ == 0U ? 0 : historyMaximum_;
}

ChartSample ChartSeries::historyAverage() const {
    return historyCount_ == 0U
        ? 0
        : static_cast<ChartSample>(historySum_ / historyCount_);
}

void ChartSeries::recalculateExtremes() {
    minimum_ = etl::numeric_limits<ChartSample>::max();
    maximum_ = etl::numeric_limits<ChartSample>::min();
    for (size_t offset = 0U; offset < count_; ++offset) {
        const ChartSample sample = sampleFromNewest(offset);
        minimum_ = etl::min(minimum_, sample);
        maximum_ = etl::max(maximum_, sample);
    }
}
