#include "precise_sleep.h"

void precise_sleep(double seconds)
{
    using namespace std;
    using namespace std::chrono;

    static double estimate = 5e-3;
    static double mean = 5e-3;
    static double m2 = 0;
    static int64_t count = 1;

    while (seconds > estimate) {
        const auto start = high_resolution_clock::now();
        this_thread::sleep_for(milliseconds(1));
        const auto end = high_resolution_clock::now();

        const double observed = (end - start).count() / 1e9;
        seconds -= observed;

        ++count;
        const double delta = observed - mean;
        mean += delta / count;
        m2 += delta * (observed - mean);
        const double stddev = sqrt(m2 / (count - 1));
        estimate = mean + stddev;
    }

    // spin lock
    auto start = high_resolution_clock::now();
    while ((high_resolution_clock::now() - start).count() / 1e9 < seconds);
}
