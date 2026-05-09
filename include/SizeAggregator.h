#pragma once

/**
 * @class SizeAggregator
 * @brief Aggregates file sizes in the file system tree structure.
 *
 * This class provides a static method to compute the aggregated file sizes.
 */
class SizeAggregator {
public:
    /**
     * @brief Computes the aggregated file sizes.
     *
     * This method uses a Reverse Linear Sweep to efficiently aggregate file sizes.
     */
    static void compute();
};