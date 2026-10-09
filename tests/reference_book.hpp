#pragma once

#include <algorithm>
#include <cstdint>
#include <vector>

#include "te/command.hpp"
#include "te/order.hpp"
#include "te/order_book.hpp"
#include "te/trade.hpp"
#include "te/types.hpp"
#include "te/validation.hpp"

namespace te::test {

// ReferenceBook is a naive, brute-force order book used as a test oracle.
// All orders are stored in a simple vector. Matching does a linear scan and sort.
// Complexity: O(N log N) per match, but obviously correct.
class ReferenceBook {
public:
    explicit ReferenceBook(SymbolId symbol) noexcept : symbol_(symbol) {}

    MatchResult add(const NewOrder& order) {
        const auto reject = validate(order);
        if (reject != RejectReason::None) {
            return MatchResult{.trades = {}, .reject = reject, .unfilled = order.quantity};
        }

        for (const auto& o : resting_) {
            if (o.id == order.id) {
                return MatchResult{.trades = {}, .reject = RejectReason::DuplicateId, .unfilled = order.quantity};
            }
        }

        MatchResult result;
        Order incoming{
            .id = order.id,
            .price = order.price,
            .timestamp = order.timestamp,
            .quantity = order.quantity,
            .remaining = order.quantity,
            .symbol = symbol_,
            .side = order.side,
            .type = order.type
        };

        if (incoming.side == Side::Buy) {
            // Find all matching asks
            std::vector<std::size_t> matchable_indices;
            for (std::size_t i = 0; i < resting_.size(); ++i) {
                if (resting_[i].side == Side::Sell) {
                    if (incoming.type == OrderType::Market || resting_[i].price <= incoming.price) {
                        matchable_indices.push_back(i);
                    }
                }
            }

            // Sort: best ask first (lowest price ASC), then earliest timestamp ASC
            std::stable_sort(matchable_indices.begin(), matchable_indices.end(),
                             [this](std::size_t a, std::size_t b) {
                                 if (resting_[a].price != resting_[b].price) {
                                     return resting_[a].price < resting_[b].price;
                                 }
                                 return resting_[a].timestamp < resting_[b].timestamp;
                             });

            for (std::size_t idx : matchable_indices) {
                if (incoming.remaining == 0) break;

                Order& resting = resting_[idx];
                const Quantity fill_qty = std::min(incoming.remaining, resting.remaining);

                result.trades.push_back(Trade{
                    .trade_id = next_trade_id_++,
                    .buy_order_id = incoming.id,
                    .sell_order_id = resting.id,
                    .price = resting.price,
                    .timestamp = incoming.timestamp,
                    .quantity = fill_qty,
                    .symbol = symbol_
                });

                incoming.remaining -= fill_qty;
                resting.remaining -= fill_qty;
            }

            // Remove fully filled orders
            resting_.erase(
                std::remove_if(resting_.begin(), resting_.end(), [](const Order& o) { return o.remaining == 0; }),
                resting_.end());

            if (incoming.remaining > 0) {
                if (incoming.type == OrderType::Limit) {
                    resting_.push_back(incoming);
                } else {
                    result.unfilled = incoming.remaining;
                }
            }
        } else {
            // Sell order matching against bids
            std::vector<std::size_t> matchable_indices;
            for (std::size_t i = 0; i < resting_.size(); ++i) {
                if (resting_[i].side == Side::Buy) {
                    if (incoming.type == OrderType::Market || resting_[i].price >= incoming.price) {
                        matchable_indices.push_back(i);
                    }
                }
            }

            // Sort: best bid first (highest price DESC), then earliest timestamp ASC
            std::stable_sort(matchable_indices.begin(), matchable_indices.end(),
                             [this](std::size_t a, std::size_t b) {
                                 if (resting_[a].price != resting_[b].price) {
                                     return resting_[a].price > resting_[b].price;
                                 }
                                 return resting_[a].timestamp < resting_[b].timestamp;
                             });

            for (std::size_t idx : matchable_indices) {
                if (incoming.remaining == 0) break;

                Order& resting = resting_[idx];
                const Quantity fill_qty = std::min(incoming.remaining, resting.remaining);

                result.trades.push_back(Trade{
                    .trade_id = next_trade_id_++,
                    .buy_order_id = resting.id,
                    .sell_order_id = incoming.id,
                    .price = resting.price,
                    .timestamp = incoming.timestamp,
                    .quantity = fill_qty,
                    .symbol = symbol_
                });

                incoming.remaining -= fill_qty;
                resting.remaining -= fill_qty;
            }

            resting_.erase(
                std::remove_if(resting_.begin(), resting_.end(), [](const Order& o) { return o.remaining == 0; }),
                resting_.end());

            if (incoming.remaining > 0) {
                if (incoming.type == OrderType::Limit) {
                    resting_.push_back(incoming);
                } else {
                    result.unfilled = incoming.remaining;
                }
            }
        }

        return result;
    }

    RejectReason cancel(OrderId id) {
        for (auto it = resting_.begin(); it != resting_.end(); ++it) {
            if (it->id == id) {
                resting_.erase(it);
                return RejectReason::None;
            }
        }
        return RejectReason::UnknownOrder;
    }

    MatchResult modify(OrderId id, Price new_price, Quantity new_quantity, Timestamp new_timestamp = 0) {
        if (new_quantity == 0) {
            const auto reject = cancel(id);
            return MatchResult{.trades = {}, .reject = reject, .unfilled = 0};
        }

        for (auto it = resting_.begin(); it != resting_.end(); ++it) {
            if (it->id == id) {
                if (new_price == it->price && new_quantity <= it->quantity) {
                    const Quantity reduction = it->quantity - new_quantity;
                    if (it->remaining <= reduction) {
                        resting_.erase(it);
                    } else {
                        it->remaining -= reduction;
                        it->quantity = new_quantity;
                    }
                    return MatchResult{};
                }

                // Price changed or size increased -> cancel and re-insert
                const Side side = it->side;
                const Timestamp ts = (new_timestamp != 0) ? new_timestamp : it->timestamp;
                resting_.erase(it);
                return add(NewOrder{
                    .id = id,
                    .price = new_price,
                    .timestamp = ts,
                    .quantity = new_quantity,
                    .symbol = symbol_,
                    .side = side,
                    .type = OrderType::Limit
                });
            }
        }

        return MatchResult{.trades = {}, .reject = RejectReason::UnknownOrder, .unfilled = 0};
    }

    [[nodiscard]] std::size_t order_count() const noexcept {
        return resting_.size();
    }

private:
    SymbolId symbol_{0};
    std::uint64_t next_trade_id_{1};
    std::vector<Order> resting_;
};

}  // namespace te::test
