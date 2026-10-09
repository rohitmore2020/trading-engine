#include <gtest/gtest.h>

#include "te/order_book.hpp"

TEST(OrderBookBasicTest, EmptyBookState) {
    te::OrderBook book(1);
    EXPECT_EQ(book.best_bid(), std::nullopt);
    EXPECT_EQ(book.best_ask(), std::nullopt);
    EXPECT_EQ(book.order_count(), 0);
    EXPECT_EQ(book.cancel(999), te::RejectReason::UnknownOrder);

    // Market order on empty book fills nothing; entire quantity unfilled
    te::NewOrder mkt{
        .id = 1,
        .price = 0,
        .timestamp = 10,
        .quantity = 100,
        .symbol = 1,
        .side = te::Side::Buy,
        .type = te::OrderType::Market
    };
    auto res = book.add(mkt);
    EXPECT_TRUE(res.trades.empty());
    EXPECT_EQ(res.unfilled, 100);
}

// Master Plan Worked Example 1: Multi-level sweep
TEST(OrderBookBasicTest, WorkedExample1_PartialFillAcrossLevels) {
    te::OrderBook book(1);

    // Resting asks:
    // Level 101 -> Order 1 (qty 100)
    // Level 102 -> Order 2 (qty 200)
    EXPECT_EQ(book.add(te::NewOrder{.id = 1, .price = 101, .timestamp = 1, .quantity = 100, .symbol = 1, .side = te::Side::Sell, .type = te::OrderType::Limit}).reject, te::RejectReason::None);
    EXPECT_EQ(book.add(te::NewOrder{.id = 2, .price = 102, .timestamp = 2, .quantity = 200, .symbol = 1, .side = te::Side::Sell, .type = te::OrderType::Limit}).reject, te::RejectReason::None);

    EXPECT_EQ(book.best_ask(), 101);
    EXPECT_EQ(book.order_count(), 2);

    // Incoming BUY LIMIT 102 x 150 (id 3)
    auto res = book.add(te::NewOrder{.id = 3, .price = 102, .timestamp = 3, .quantity = 150, .symbol = 1, .side = te::Side::Buy, .type = te::OrderType::Limit});

    ASSERT_EQ(res.trades.size(), 2);
    EXPECT_EQ(res.unfilled, 0);

    // Trade 1: 100 @ 101 against Order 1
    EXPECT_EQ(res.trades[0].trade_id, 1);
    EXPECT_EQ(res.trades[0].buy_order_id, 3);
    EXPECT_EQ(res.trades[0].sell_order_id, 1);
    EXPECT_EQ(res.trades[0].price, 101);
    EXPECT_EQ(res.trades[0].quantity, 100);

    // Trade 2: 50 @ 102 against Order 2
    EXPECT_EQ(res.trades[1].trade_id, 2);
    EXPECT_EQ(res.trades[1].buy_order_id, 3);
    EXPECT_EQ(res.trades[1].sell_order_id, 2);
    EXPECT_EQ(res.trades[1].price, 102);
    EXPECT_EQ(res.trades[1].quantity, 50);

    // Final book state:
    // Ask 101 exhausted. Ask 102 has 150 remaining. No bids resting.
    EXPECT_EQ(book.best_ask(), 102);
    EXPECT_EQ(book.depth_at(te::Side::Sell, 102), 150);
    EXPECT_EQ(book.best_bid(), std::nullopt);
    EXPECT_EQ(book.order_count(), 1);
}

// Master Plan Worked Example 2: Remainder rests in book
TEST(OrderBookBasicTest, WorkedExample2_RemainderRests) {
    te::OrderBook book(1);

    EXPECT_EQ(book.add(te::NewOrder{.id = 1, .price = 101, .timestamp = 1, .quantity = 100, .symbol = 1, .side = te::Side::Sell, .type = te::OrderType::Limit}).reject, te::RejectReason::None);
    EXPECT_EQ(book.add(te::NewOrder{.id = 2, .price = 102, .timestamp = 2, .quantity = 200, .symbol = 1, .side = te::Side::Sell, .type = te::OrderType::Limit}).reject, te::RejectReason::None);

    // Incoming BUY LIMIT 101 x 150 (id 4)
    auto res = book.add(te::NewOrder{.id = 4, .price = 101, .timestamp = 3, .quantity = 150, .symbol = 1, .side = te::Side::Buy, .type = te::OrderType::Limit});

    ASSERT_EQ(res.trades.size(), 1);
    EXPECT_EQ(res.trades[0].price, 101);
    EXPECT_EQ(res.trades[0].quantity, 100);

    // Remaining 50 rests as a bid at 101
    EXPECT_EQ(book.best_bid(), 101);
    EXPECT_EQ(book.depth_at(te::Side::Buy, 101), 50);
    EXPECT_EQ(book.best_ask(), 102);
    EXPECT_EQ(book.depth_at(te::Side::Sell, 102), 200);
}

// Master Plan Worked Example 3: Time Priority (FIFO)
TEST(OrderBookBasicTest, WorkedExample3_TimePriorityFIFO) {
    te::OrderBook book(1);

    // Two orders at the exact same price: Order A arrives at t1, Order C at t2
    EXPECT_EQ(book.add(te::NewOrder{.id = 10, .price = 101, .timestamp = 1, .quantity = 50, .symbol = 1, .side = te::Side::Sell, .type = te::OrderType::Limit}).reject, te::RejectReason::None);
    EXPECT_EQ(book.add(te::NewOrder{.id = 11, .price = 101, .timestamp = 2, .quantity = 70, .symbol = 1, .side = te::Side::Sell, .type = te::OrderType::Limit}).reject, te::RejectReason::None);

    // Incoming BUY 101 x 60 -> fills Order A 50 first, then Order C 10
    auto res = book.add(te::NewOrder{.id = 12, .price = 101, .timestamp = 3, .quantity = 60, .symbol = 1, .side = te::Side::Buy, .type = te::OrderType::Limit});

    ASSERT_EQ(res.trades.size(), 2);
    EXPECT_EQ(res.trades[0].sell_order_id, 10);
    EXPECT_EQ(res.trades[0].quantity, 50);

    EXPECT_EQ(res.trades[1].sell_order_id, 11);
    EXPECT_EQ(res.trades[1].quantity, 10);

    // Order C keeps its place with 60 remaining
    EXPECT_EQ(book.best_ask(), 101);
    EXPECT_EQ(book.depth_at(te::Side::Sell, 101), 60);
}

// Master Plan Worked Example 4: Market order with partial liquidity
TEST(OrderBookBasicTest, WorkedExample4_MarketOrderPartialLiquidity) {
    te::OrderBook book(1);

    // Total ask liquidity = 300
    EXPECT_EQ(book.add(te::NewOrder{.id = 1, .price = 100, .timestamp = 1, .quantity = 100, .symbol = 1, .side = te::Side::Sell, .type = te::OrderType::Limit}).reject, te::RejectReason::None);
    EXPECT_EQ(book.add(te::NewOrder{.id = 2, .price = 101, .timestamp = 2, .quantity = 200, .symbol = 1, .side = te::Side::Sell, .type = te::OrderType::Limit}).reject, te::RejectReason::None);

    // Market Buy 500: sweeps all 300, remaining 200 discarded
    auto res = book.add(te::NewOrder{.id = 3, .price = 0, .timestamp = 3, .quantity = 500, .symbol = 1, .side = te::Side::Buy, .type = te::OrderType::Market});

    ASSERT_EQ(res.trades.size(), 2);
    EXPECT_EQ(res.trades[0].quantity, 100);
    EXPECT_EQ(res.trades[1].quantity, 200);
    EXPECT_EQ(res.unfilled, 200);

    EXPECT_EQ(book.best_ask(), std::nullopt);
    EXPECT_EQ(book.best_bid(), std::nullopt);
}

TEST(OrderBookBasicTest, CancellationAndRejection) {
    te::OrderBook book(1);

    EXPECT_EQ(book.add(te::NewOrder{.id = 1, .price = 100, .timestamp = 1, .quantity = 100, .symbol = 1, .side = te::Side::Buy, .type = te::OrderType::Limit}).reject, te::RejectReason::None);
    EXPECT_EQ(book.order_count(), 1);

    // Duplicate ID rejected
    auto dup = book.add(te::NewOrder{.id = 1, .price = 101, .timestamp = 2, .quantity = 50, .symbol = 1, .side = te::Side::Buy, .type = te::OrderType::Limit});
    EXPECT_EQ(dup.reject, te::RejectReason::DuplicateId);

    // Cancel order 1
    EXPECT_EQ(book.cancel(1), te::RejectReason::None);
    EXPECT_EQ(book.order_count(), 0);
    EXPECT_EQ(book.best_bid(), std::nullopt);

    // Cancel non-existent
    EXPECT_EQ(book.cancel(1), te::RejectReason::UnknownOrder);
}

TEST(OrderBookBasicTest, ModificationPrioritySemantics) {
    te::OrderBook book(1);

    // Orders A (id 1) and B (id 2) at price 100
    EXPECT_EQ(book.add(te::NewOrder{.id = 1, .price = 100, .timestamp = 1, .quantity = 100, .symbol = 1, .side = te::Side::Buy, .type = te::OrderType::Limit}).reject, te::RejectReason::None);
    EXPECT_EQ(book.add(te::NewOrder{.id = 2, .price = 100, .timestamp = 2, .quantity = 100, .symbol = 1, .side = te::Side::Buy, .type = te::OrderType::Limit}).reject, te::RejectReason::None);

    // Reduce order 1 quantity to 50 (in-place decrease keeps queue priority)
    auto mod = book.modify(1, 100, 50);
    EXPECT_EQ(mod.reject, te::RejectReason::None);
    EXPECT_EQ(book.depth_at(te::Side::Buy, 100), 150);

    // Incoming sell of 60: Order 1 must fill 50 first (proves it kept priority!)
    auto match = book.add(te::NewOrder{.id = 3, .price = 100, .timestamp = 3, .quantity = 60, .symbol = 1, .side = te::Side::Sell, .type = te::OrderType::Limit});
    ASSERT_EQ(match.trades.size(), 2);
    EXPECT_EQ(match.trades[0].buy_order_id, 1);
    EXPECT_EQ(match.trades[0].quantity, 50);
    EXPECT_EQ(match.trades[1].buy_order_id, 2);
    EXPECT_EQ(match.trades[1].quantity, 10);
}
