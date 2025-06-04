#include "orderbook/FixedDepthOrderBook.hpp"
#include <stdexcept>

namespace orderbook {

FixedDepthOrderBook::FixedDepthOrderBook(std::size_t maxDepth)
    : maxDepth_(maxDepth) {
    if (maxDepth == 0) {
        throw std::invalid_argument("maxDepth must be greater than 0");
    }
}

void FixedDepthOrderBook::applyUpdate(const BookUpdateEvent& event) {
    if (event.type == BookUpdateEvent::Type::SNAPSHOT) {
        applySnapshotUpdate(event);
    } else {
        applyDeltaUpdate(event);
    }
}

void FixedDepthOrderBook::applySnapshotUpdate(const BookUpdateEvent& event) {
    auto& levels = (event.side == Side::BID) ? bids_ : asks_;
    
    if (event.volume > 0) {
        levels[event.price] = event.volume;
        truncateToMaxDepth(event.side);
    } else {
        levels.erase(event.price);
    }
}

void FixedDepthOrderBook::applyDeltaUpdate(const BookUpdateEvent& event) {
    auto& levels = (event.side == Side::BID) ? bids_ : asks_;
    
    auto it = levels.find(event.price);
    if (it != levels.end()) {
        double newVolume = it->second + event.volume;
        if (newVolume <= 0) {
            levels.erase(it);
        } else {
            it->second = newVolume;
        }
    } else if (event.volume > 0) {
        levels[event.price] = event.volume;
        truncateToMaxDepth(event.side);
    }
}

void FixedDepthOrderBook::truncateToMaxDepth(Side side) {
    auto& levels = (side == Side::BID) ? bids_ : asks_;
    while (levels.size() > maxDepth_) {
        if (side == Side::BID) {
            levels.erase(std::prev(levels.end())); // Remove worst bid
        } else {
            levels.erase(std::prev(levels.end())); // Remove worst ask
        }
    }
}

double FixedDepthOrderBook::getBestBid() const {
    return bids_.empty() ? 0.0 : bids_.begin()->first;
}

double FixedDepthOrderBook::getBestAsk() const {
    return asks_.empty() ? 0.0 : asks_.begin()->first;
}

double FixedDepthOrderBook::getVolumeAtPrice(Side side, double price) const {
    const auto& levels = (side == Side::BID) ? bids_ : asks_;
    auto it = levels.find(price);
    return (it != levels.end()) ? it->second : 0.0;
}

std::vector<PriceLevel> FixedDepthOrderBook::getLevels(Side side) const {
    std::vector<PriceLevel> result;
    const auto& levels = (side == Side::BID) ? bids_ : asks_;
    
    result.reserve(levels.size());
    for (const auto& [price, volume] : levels) {
        result.push_back({price, volume});
    }
    
    return result;
}

std::size_t FixedDepthOrderBook::getMaxDepth() const {
    return maxDepth_;
}

void FixedDepthOrderBook::clear() {
    bids_.clear();
    asks_.clear();
}

} // namespace orderbook 