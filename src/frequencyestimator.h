#ifndef FREQUENCYESTIMATOR_H
#define FREQUENCYESTIMATOR_H

#include <QString>
#include <cstdint>
#include <optional>

/// Estimates average frequency of events based on last N samples
class FrequencyEstimator
{
public:
    explicit FrequencyEstimator();

    /// Time from an arbitrary epoch, in microseconds
    void addTime(int64_t updateTime);
    double frequency() const;
    QString formatFrequency() const;

private:
    size_t sampleCount() const;

    static constexpr size_t BUFFER_LENGTH = 10;
    std::optional<int64_t> buffer[BUFFER_LENGTH];
    size_t nextIndex = 0;
};

#endif // FREQUENCYESTIMATOR_H
