#include <gtest/gtest.h>

#include "te/order_book.hpp"

TEST(InvariantTest, EmptyBookPassesInvariants) {
    te::OrderBook book(1);
    EXPECT_NO_THROW(book.check_invariants());
}

TEST(InvariantTest, NonCrossingOrdersMaintainInvariants) {
    te::OrderBook book(1);

    // Add bids
    EXPECT_EQ(book.add(te::NewOrder{.id = 1, .price = 100, .timestamp = 10, .quantity = 50, .symbol = 1, .side = te::Side::Buy, .type = te::OrderType::Limit}).reject, te::RejectReason::None);
    EXPECT_NO_THROW(book.check_invariants());

    EXPECT_EQ(book.add(te::NewOrder{.id = 2, .price = 99, .timestamp = 11, .quantity = 100, .symbol = 1, .side = te::Side::Buy, .type = te::OrderType::Limit}).reject, te::RejectReason::None);
    EXPECT_NO_THROW(book.check_invariants());

    // Add asks
    EXPECT_EQ(book.add(te::NewOrder{.id = 3, .price = 101, .timestamp = 12, .quantity = 75, .symbol = 1, .side = te::Side::Sell, .type = te::OrderType::Limit}).reject, te::RejectReason::None);
    EXPECT_NO_THROW(book.check_invariants());

    EXPECT_EQ(book.add(te::NewOrder{.id = 4, .price = 102, .timestamp = 13, .quantity = 200, .symbol = 1, .side = te::Side::Sell, .type = te::OrderType::Limit}).reject, te::RejectReason::None);
    EXPECT_NO_THROW(book.check_invariants());

    EXPECT_EQ(book.order_count(), 4);
    EXPECT_EQ(book.depth_at(te::Side::Buy, 100), 50);
    EXPECT_EQ(book.depth_at(te::Side::Buy, 99), 100);
    EXPECT_EQ(book.depth_at(te::Side::Sell, 101), 75);
    EXPECT_EQ(book.depth_at(te::Side::Sell, 102), 200);
}

TEST(InvariantTest, InvariantsHoldAcrossPartialFillsAndLevelExhaustion) {
    te::OrderBook book(1);

    // Seed multiple levels
    static_cast<void>(book.add(te::NewOrder{.id = 1, .price = 100, .timestamp = 1, .quantity = 50, .symbol = 1, .side = te::Side::Buy, .type = te::OrderType::Limit}));
    static_cast<void>(book.add(te::NewOrder{.id = 2, .price = 100, .timestamp = 2, .quantity = 50, .symbol = 1, .side = te::Side::Buy, .type = te::OrderType::Limit}));
    static_cast<void>(book.add(te::NewOrder{.id = 3, .price = 99, .timestamp = 3, .quantity = 100, .symbol = 1, .side = te::Side::Buy, .type = te::OrderType::Limit}));

    // Sell sweeps level 100 partially
    auto res1 = book.add(te::NewOrder{.id = 10, .price = 100, .timestamp = 4, .quantity = 75, .symbol = 1, .side = te::Side::Sell, .type = te::OrderType::Limit});
    EXPECT_EQ(res1.trades.size(), 2);
    EXPECT_NO_THROW(book.check_invariants());
    EXPECT_EQ(book.depth_at(te::Side::Buy, 100), 25);

    // Sell finishes level 100 completely -> level 100 must be deleted cleanly
    auto res2 = book.add(te::NewOrder{.id = 11, .price = 99, .timestamp = 5, .quantity = 50, .symbol = 1, .side = te::Side::Sell, .type = te::OrderType::Limit});
    EXPECT_EQ(res2.trades.size(), 2);  // fills remaining 25 @ 100 and 25 @ 99
    EXPECT_NO_THROW(book.check_invariants());
    EXPECT_EQ(book.best_bid(), 99);
    EXPECT_EQ(book.depth_at(te::Side::Buy, 100), 0);
    EXPECT_EQ(book.depth_at(te::Side::Buy, 99), 75);
}

TEST(InvariantTest, CancellationPreservesInvariants) {
    te::OrderBook book(1);

    static_cast<void>(book.add(te::NewOrder{.id = 1, .price = 100, .timestamp = 1, .quantity = 50, .symbol = 1, .side = te::Side::Buy, .type = te::OrderType::Limit}));
    static_cast<void>(book.add(te::NewOrder{.id = 2, .price = 100, .timestamp = 2, .quantity = 70, .symbol = 1, .side = te::Side::Buy, .type = te::OrderType::Limit}));
    static_cast<void>(book.add(te::NewOrder{.id = 3, .price = 100, .timestamp = 3, .quantity = 90, .symbol = 1, .side = te::Side::Buy, .type = te::OrderType::Limit}));
    EXPECT_NO_THROW(book.check_invariants());

    // Cancel middle order
    EXPECT_EQ(book.cancel(2), te::RejectReason::None);
    EXPECT_NO_THROW(book.check_invariants());
    EXPECT_EQ(book.depth_at(te::Side::Buy, 100), 140);

    // Cancel front order
    EXPECT_EQ(book.cancel(1), te::RejectReason::None);
    EXPECT_NO_THROW(book.check_invariants());
    EXPECT_EQ(book.depth_at(te::Side::Buy, 100), 90);

    // Cancel last order -> level deleted
    EXPECT_EQ(book.cancel(3), te::RejectReason::None);
    EXPECT_NO_THROW(book.check_invariants());
    EXPECT_EQ(book.depth_at(te::Side::Buy, 100), 0);
    EXPECT_EQ(book.best_bid(), std::nullopt);
}
