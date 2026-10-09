#include <gtest/gtest.h>

#include <random>
#include <vector>

#include "te/order_book.hpp"

TEST(StressTest, HeavyCancellationWorkload) {
    te::OrderBook book(1);
    std::mt19937_64 rng(1337);
    std::uniform_int_distribution<int> side_dist(0, 1);
    std::uniform_int_distribution<int> price_dist(100, 150);
    std::uniform_int_distribution<int> qty_dist(10, 100);

    std::vector<te::OrderId> active_ids;
    te::OrderId next_id = 1;

    // 80% Cancel, 20% Add
    for (int i = 0; i < 20000; ++i) {
        if (active_ids.empty() || (i % 5 == 0)) {
            // Add order
            const te::OrderId id = next_id++;
            active_ids.push_back(id);
            static_cast<void>(book.add(te::NewOrder{
                .id = id,
                .price = price_dist(rng),
                .timestamp = static_cast<te::Timestamp>(i),
                .quantity = static_cast<te::Quantity>(qty_dist(rng)),
                .symbol = 1,
                .side = (side_dist(rng) == 0) ? te::Side::Buy : te::Side::Sell,
                .type = te::OrderType::Limit
            }));
        } else {
            // Cancel order
            std::uniform_int_distribution<std::size_t> idx_dist(0, active_ids.size() - 1);
            const std::size_t idx = idx_dist(rng);
            static_cast<void>(book.cancel(active_ids[idx]));
            active_ids.erase(active_ids.begin() + static_cast<std::ptrdiff_t>(idx));
        }

        if (i % 2000 == 0) {
            EXPECT_NO_THROW(book.check_invariants());
        }
    }

    EXPECT_NO_THROW(book.check_invariants());
}

TEST(StressTest, GiantAggressiveSweep) {
    te::OrderBook book(1);

    // Seed 100 ask levels from 101 to 200, each with 50 shares
    for (te::Price p = 101; p <= 200; ++p) {
        static_cast<void>(book.add(te::NewOrder{
            .id = static_cast<te::OrderId>(p),
            .price = p,
            .timestamp = static_cast<te::Timestamp>(p),
            .quantity = 50,
            .symbol = 1,
            .side = te::Side::Sell,
            .type = te::OrderType::Limit
        }));
    }

    EXPECT_EQ(book.order_count(), 100);
    EXPECT_EQ(book.best_ask(), 101);

    // One giant Buy order sweeping all 100 levels (5000 shares total)
    auto res = book.add(te::NewOrder{
        .id = 99999,
        .price = 250,
        .timestamp = 5000,
        .quantity = 5000,
        .symbol = 1,
        .side = te::Side::Buy,
        .type = te::OrderType::Limit
    });

    EXPECT_EQ(res.trades.size(), 100);
    EXPECT_EQ(book.order_count(), 0);
    EXPECT_EQ(book.best_ask(), std::nullopt);
    EXPECT_NO_THROW(book.check_invariants());
}

TEST(StressTest, HighOrderCountNarrowBand) {
    te::OrderBook book(1);

    // 10,000 orders resting across just 5 price levels (2,000 orders per level)
    for (te::OrderId id = 1; id <= 10000; ++id) {
        const te::Price px = 100 + static_cast<te::Price>(id % 5);
        static_cast<void>(book.add(te::NewOrder{
            .id = id,
            .price = px,
            .timestamp = static_cast<te::Timestamp>(id),
            .quantity = 10,
            .symbol = 1,
            .side = te::Side::Buy,
            .type = te::OrderType::Limit
        }));
    }

    EXPECT_EQ(book.order_count(), 10000);
    EXPECT_NO_THROW(book.check_invariants());

    // Sweep all 10,000 orders with a market sell of 100,000 shares
    auto res = book.add(te::NewOrder{
        .id = 999999,
        .price = 0,
        .timestamp = 20000,
        .quantity = 100000,
        .symbol = 1,
        .side = te::Side::Sell,
        .type = te::OrderType::Market
    });

    EXPECT_EQ(res.trades.size(), 10000);
    EXPECT_EQ(book.order_count(), 0);
    EXPECT_NO_THROW(book.check_invariants());
}
