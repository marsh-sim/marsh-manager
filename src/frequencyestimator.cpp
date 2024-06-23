#include "frequencyestimator.h"

FrequencyEstimator::FrequencyEstimator()
{
    for (int i = 0; i < BUFFER_LENGTH; ++i) {
        buffer[i].reset();
    }
}

void FrequencyEstimator::addTime(int64_t updateTime)
{
    buffer[nextIndex] = updateTime;
    nextIndex = (nextIndex + 1) % BUFFER_LENGTH;
}

double FrequencyEstimator::frequency() const
{
    const auto count = sampleCount();
    int64_t first, last;
    if (count <= 1) {
        // aperiodic event
        return qQNaN();
    } else if (count < BUFFER_LENGTH) {
        // filling buffer first time
        first = *buffer[0];
        last = *buffer[count - 1];
    } else {
        // ring buffer usage
        first = *buffer[nextIndex]; // oldest sample is next to be overwritten
        last = *buffer[(nextIndex + BUFFER_LENGTH - 1) % BUFFER_LENGTH]; // previous without underflow
    }
    double meanInterval = static_cast<double>(last - first) / (count - 1);
    return 1e6 / meanInterval; // from microsecond to Hertz
}

QString FrequencyEstimator::formatFrequency() const
{
    const auto count = sampleCount();
    if (count == 0) {
        return "(never)";
    } else if (count == 1) {
        return "(once)";
    } else if (frequency() >= 0.1) {
        return QString("%1 Hz").arg(frequency(), 0, 'g', 3);
    } else {
        return QString("T≈%1 s").arg(1.0 / frequency(), 0, 'f', 1);
    }
}

size_t FrequencyEstimator::sampleCount() const
{
    size_t count = 0;
    for (int i = 0; i < BUFFER_LENGTH; ++i) {
        if (buffer[i])
            count++;
    }
    return count;
}
