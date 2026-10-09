#pragma once

#include <cstdint>
#include <functional>
#include <list>
#include <map>
#include <optional>
#include <unordered_map>
#include <vector>

#include "te/command.hpp"
#include "te/order.hpp"
#include "te/trade.hpp"
#include "te/types.hpp"

namespace te {

struct PriceLevel {
    Price price{0};
    Quantity total_quantity{0};
    std::list<Order> orders;
};

struct OrderLocator {
    Side side{Side::Buy};
    Price price{0};
    std::list<Order>::iterator it;
};

struct MatchResult {
    std::vector<Trade> trades;
    RejectReason reject{RejectReason::None};
    Quantity unfilled{0};
};

class OrderBook {
public:
    explicit OrderBook(SymbolId symbol) noexcept;

    // Disallow copying to prevent accidental heavy allocations; allow moving
    OrderBook(const OrderBook&) = delete;
    OrderBook& operator=(const OrderBook&) = delete;
    OrderBook(OrderBook&&) noexcept = default;
    OrderBook& operator=(OrderBook&&) noexcept = default;
    ~OrderBook() = default;

    // Core trading operations
    [[nodiscard]] MatchResult add(const NewOrder& order);
    [[nodiscard]] RejectReason cancel(OrderId id);
    [[nodiscard]] MatchResult modify(OrderId id, Price new_price, Quantity new_quantity);

    // Queries
    [[nodiscard]] std::optional<Price> best_bid() const noexcept;
    [[nodiscard]] std::optional<Price> best_ask() const noexcept;
    [[nodiscard]] Quantity depth_at(Side side, Price price) const noexcept;
    [[nodiscard]] std::size_t order_count() const noexcept;
    [[nodiscard]] SymbolId symbol() const noexcept;

private:
    template <typename OppositeMap, typename CrossesPredicate>
    void match_against(Order& incoming,
                       OppositeMap& opposite_levels,
                       CrossesPredicate&& crosses,
                       std::vector<Trade>& trades);

    void rest_order(const Order& order);

    SymbolId symbol_{0};
    std::uint64_t next_trade_id_{1};

    // Bids sorted descending: begin() is highest/best bid
    std::map<Price, PriceLevel, std::greater<Price>> bids_;

    // Asks sorted ascending: begin() is lowest/best ask
    std::map<Price, PriceLevel, std::less<Price>> asks_;

    // O(1) order lookup by OrderId
    std::unordered_map<OrderId, OrderLocator> index_;
};

}  // namespace te
