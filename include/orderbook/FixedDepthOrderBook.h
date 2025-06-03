#pragma once

#include <map>
#include <cstdint>
#include <stdexcept>

namespace orderbook {

enum class Side {
    BID,
    ASK
};

struct BookUpdateEvent {
    enum class Type {
        SNAPSHOT,
        DELTA
    };

    Type type;
    Side side;
    double price;
    double volume;
};

class IOrderBook {
public:
    virtual ~IOrderBook() = default;
    virtual void applyUpdate(const BookUpdateEvent& event) = 0;
    virtual double getBestBid() const = 0;
    virtual double getBestAsk() const = 0;
    virtual double getVolumeAtPrice(Side side, double price) const = 0;
};

class FixedDepthOrderBook : public IOrderBook {
public:
    explicit FixedDepthOrderBook(size_t maxDepth);
    
    void applyUpdate(const BookUpdateEvent& event) override;
    double getBestBid() const override;
    double getBestAsk() const override;
    double getVolumeAtPrice(Side side, double price) const override;

private:
    void applySnapshotUpdate(const BookUpdateEvent& event);
    void applyDeltaUpdate(const BookUpdateEvent& event);
    void truncateToMaxDepth(Side side);

    const size_t maxDepth_;
    std::map<double, double, std::greater<>> bids_; // Price -> Volume, descending order
    std::map<double, double> asks_;                 // Price -> Volume, ascending order
}; 