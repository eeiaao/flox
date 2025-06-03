#pragma once

#include "IOrderBook.hpp"
#include <map>
#include <algorithm>

namespace orderbook {

class FixedDepthOrderBook : public IOrderBook {
public:
    explicit FixedDepthOrderBook(std::size_t maxDepth);
    ~FixedDepthOrderBook() override = default;

    void applyUpdate(const BookUpdateEvent& update) override;
    double getBestBid() const override;
    double getBestAsk() const override;
    double getVolumeAtPrice(Side side, double price) const override;
    std::vector<PriceLevel> getLevels(Side side) const override;
    std::size_t getMaxDepth() const override;
    void clear() override;

private:
    void truncateToMaxDepth(Side side);
    void applyDeltaUpdate(const std::vector<PriceLevel>& bids, const std::vector<PriceLevel>& asks);
    void applySnapshotUpdate(const std::vector<PriceLevel>& bids, const std::vector<PriceLevel>& asks);

    std::map<double, double, std::greater<>> bids_; // Price -> Volume (descending order)
    std::map<double, double, std::less<>> asks_;    // Price -> Volume (ascending order)
    std::size_t maxDepth_;
};

} // namespace orderbook 