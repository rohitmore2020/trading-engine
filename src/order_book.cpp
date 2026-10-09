#include "te/order_book.hpp"

#include <algorithm>
#include <utility>

#include "te/validation.hpp"

namespace te {

OrderBook::OrderBook(SymbolId symbol) noexcept : symbol_(symbol) {}

MatchResult OrderBook::add(const NewOrder& order) {
    // 1. Input validation
    const auto reject = validate(order);
    if (reject != RejectReason::None) {
        return MatchResult{.trades = {}, .reject = reject, .unfilled = order.quantity};
    }

    // 2. Reject duplicate order ID
    if (index_.find(order.id) != index_.end()) {
        return MatchResult{.trades = {}, .reject = RejectReason::DuplicateId, .unfilled = order.quantity};
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

    // 3. Match against opposite side
    if (incoming.side == Side::Buy) {
        match_against(
            incoming, asks_,
            [](Price ask_price, Price limit_price, OrderType type) noexcept {
                return type == OrderType::Market || ask_price <= limit_price;
            },
            result.trades);
    } else {
        match_against(
            incoming, bids_,
            [](Price bid_price, Price limit_price, OrderType type) noexcept {
                return type == OrderType::Market || bid_price >= limit_price;
            },
            result.trades);
    }

    // 4. Handle remainder
    if (incoming.remaining > 0) {
        if (incoming.type == OrderType::Limit) {
            rest_order(incoming);
        } else {
            // Market orders discard unfilled remainder (IOC semantics)
            result.unfilled = incoming.remaining;
        }
    }

    return result;
}

RejectReason OrderBook::cancel(OrderId id) {
    const auto it = index_.find(id);
    if (it == index_.end()) {
        return RejectReason::UnknownOrder;
    }

    const OrderLocator loc = it->second;
    if (loc.side == Side::Buy) {
        const auto level_it = bids_.find(loc.price);
        if (level_it != bids_.end()) {
            level_it->second.total_quantity -= loc.it->remaining;
            level_it->second.orders.erase(loc.it);
            if (level_it->second.orders.empty()) {
                bids_.erase(level_it);
            }
        }
    } else {
        const auto level_it = asks_.find(loc.price);
        if (level_it != asks_.end()) {
            level_it->second.total_quantity -= loc.it->remaining;
            level_it->second.orders.erase(loc.it);
            if (level_it->second.orders.empty()) {
                asks_.erase(level_it);
            }
        }
    }

    index_.erase(it);
    return RejectReason::None;
}

MatchResult OrderBook::modify(OrderId id, Price new_price, Quantity new_quantity) {
    if (new_quantity == 0) {
        const auto reject = cancel(id);
        return MatchResult{.trades = {}, .reject = reject, .unfilled = 0};
    }

    const auto it = index_.find(id);
    if (it == index_.end()) {
        return MatchResult{.trades = {}, .reject = RejectReason::UnknownOrder, .unfilled = 0};
    }

    const OrderLocator loc = it->second;
    const Side side = loc.side;

    // In-place quantity decrease preserves price-time priority
    if (new_price == loc.price && new_quantity <= loc.it->quantity) {
        const Quantity reduction = loc.it->quantity - new_quantity;
        if (loc.it->remaining <= reduction) {
            // Entire remaining portion canceled
            static_cast<void>(cancel(id));
        } else {
            loc.it->remaining -= reduction;
            loc.it->quantity = new_quantity;

            if (side == Side::Buy) {
                bids_[loc.price].total_quantity -= reduction;
            } else {
                asks_[loc.price].total_quantity -= reduction;
            }
        }
        return MatchResult{};
    }

    // Otherwise (price changed or quantity increased): Cancel and replace at back of queue
    const Timestamp ts = loc.it->timestamp;
    static_cast<void>(cancel(id));

    NewOrder replacement{
        .id = id,
        .price = new_price,
        .timestamp = ts,
        .quantity = new_quantity,
        .symbol = symbol_,
        .side = side,
        .type = OrderType::Limit
    };

    return add(replacement);
}

std::optional<Price> OrderBook::best_bid() const noexcept {
    if (bids_.empty()) {
        return std::nullopt;
    }
    return bids_.begin()->first;
}

std::optional<Price> OrderBook::best_ask() const noexcept {
    if (asks_.empty()) {
        return std::nullopt;
    }
    return asks_.begin()->first;
}

Quantity OrderBook::depth_at(Side side, Price price) const noexcept {
    if (side == Side::Buy) {
        const auto it = bids_.find(price);
        return it != bids_.end() ? it->second.total_quantity : 0;
    }
    const auto it = asks_.find(price);
    return it != asks_.end() ? it->second.total_quantity : 0;
}

std::size_t OrderBook::order_count() const noexcept {
    return index_.size();
}

SymbolId OrderBook::symbol() const noexcept {
    return symbol_;
}

template <typename OppositeMap, typename CrossesPredicate>
void OrderBook::match_against(Order& incoming,
                              OppositeMap& opposite_levels,
                              CrossesPredicate&& crosses,
                              std::vector<Trade>& trades) {
    while (incoming.remaining > 0 && !opposite_levels.empty()) {
        auto level_it = opposite_levels.begin();
        const Price level_price = level_it->first;

        // Stop if price does not cross
        if (!crosses(level_price, incoming.price, incoming.type)) {
            break;
        }

        PriceLevel& level = level_it->second;

        while (incoming.remaining > 0 && !level.orders.empty()) {
            Order& resting = level.orders.front();
            const Quantity fill_qty = std::min(incoming.remaining, resting.remaining);

            const OrderId buy_id = incoming.side == Side::Buy ? incoming.id : resting.id;
            const OrderId sell_id = incoming.side == Side::Sell ? incoming.id : resting.id;

            // Trades execute at the resting (passive) order's price
            trades.push_back(Trade{
                .trade_id = next_trade_id_++,
                .buy_order_id = buy_id,
                .sell_order_id = sell_id,
                .price = level_price,
                .timestamp = incoming.timestamp,
                .quantity = fill_qty,
                .symbol = symbol_
            });

            incoming.remaining -= fill_qty;
            resting.remaining -= fill_qty;
            level.total_quantity -= fill_qty;

            if (resting.remaining == 0) {
                index_.erase(resting.id);
                level.orders.pop_front();
            }
        }

        if (level.orders.empty()) {
            opposite_levels.erase(level_it);
        }
    }
}

void OrderBook::rest_order(const Order& order) {
    if (order.side == Side::Buy) {
        auto& level = bids_[order.price];
        level.price = order.price;
        level.total_quantity += order.remaining;
        level.orders.push_back(order);

        // Store iterator to the newly inserted order for O(1) cancel
        auto it = std::prev(level.orders.end());
        index_[order.id] = OrderLocator{.side = Side::Buy, .price = order.price, .it = it};
    } else {
        auto& level = asks_[order.price];
        level.price = order.price;
        level.total_quantity += order.remaining;
        level.orders.push_back(order);

        auto it = std::prev(level.orders.end());
        index_[order.id] = OrderLocator{.side = Side::Sell, .price = order.price, .it = it};
    }
}

}  // namespace te
