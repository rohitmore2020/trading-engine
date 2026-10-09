#include <gtest/gtest.h>

#include "te/order_book.hpp"

TEST(CancelModifyTest, CancelPartiallyFilledOrder) {
    te::OrderBook book(1);

    // Limit Buy 100 @ 100
    static_cast<void>(book.add(te::NewOrder{.id = 1, .price = 100, .timestamp = 1, .quantity = 100, .symbol = 1, .side = te::Side::Buy, .type = te::OrderType::Limit}));

    // Sell 40 @ 100 -> Order 1 partially filled (60 remaining)
    auto res = book.add(te::NewOrder{.id = 2, .price = 100, .timestamp = 2, .quantity = 40, .symbol = 1, .side = te::Side::Sell, .type = te::OrderType::Limit});
    EXPECT_EQ(res.trades.size(), 1);
    EXPECT_EQ(book.depth_at(te::Side::Buy, 100), 60);

    // Cancel remainder
    EXPECT_EQ(book.cancel(1), te::RejectReason::None);
    EXPECT_EQ(book.depth_at(te::Side::Buy, 100), 0);
    EXPECT_EQ(book.best_bid(), std::nullopt);
    EXPECT_NO_THROW(book.check_invariants());
}

TEST(CancelModifyTest, ModifyPriceToCrossingExecutesImmediateMatch) {
    te::OrderBook book(1);

    // Resting Ask: 100 @ 102
    static_cast<void>(book.add(te::NewOrder{.id = 1, .price = 102, .timestamp = 1, .quantity = 100, .symbol = 1, .side = te::Side::Sell, .type = te::OrderType::Limit}));

    // Resting Bid: 50 @ 100
    static_cast<void>(book.add(te::NewOrder{.id = 2, .price = 100, .timestamp = 2, .quantity = 50, .symbol = 1, .side = te::Side::Buy, .type = te::OrderType::Limit}));

    EXPECT_EQ(book.best_bid(), 100);
    EXPECT_EQ(book.best_ask(), 102);

    // Modify Bid 2 from 100 -> 102 (now crosses Ask 1!)
    auto mod_res = book.modify(2, 102, 50, 3);
    ASSERT_EQ(mod_res.trades.size(), 1);
    EXPECT_EQ(mod_res.trades[0].buy_order_id, 2);
    EXPECT_EQ(mod_res.trades[0].sell_order_id, 1);
    EXPECT_EQ(mod_res.trades[0].price, 102);  // Trades at resting ask price!
    EXPECT_EQ(mod_res.trades[0].quantity, 50);

    // Bid is filled; Ask has 50 remaining @ 102
    EXPECT_EQ(book.best_bid(), std::nullopt);
    EXPECT_EQ(book.best_ask(), 102);
    EXPECT_EQ(book.depth_at(te::Side::Sell, 102), 50);
    EXPECT_NO_THROW(book.check_invariants());
}

TEST(CancelModifyTest, ModifyQuantityIncreaseLosesPriority) {
    te::OrderBook book(1);

    // Order 1 arrives at t=1, Order 2 arrives at t=2 at same price 100
    static_cast<void>(book.add(te::NewOrder{.id = 1, .price = 100, .timestamp = 1, .quantity = 50, .symbol = 1, .side = te::Side::Buy, .type = te::OrderType::Limit}));
    static_cast<void>(book.add(te::NewOrder{.id = 2, .price = 100, .timestamp = 2, .quantity = 50, .symbol = 1, .side = te::Side::Buy, .type = te::OrderType::Limit}));

    // Increase Order 1 quantity to 100 at t=3 -> loses queue priority, moves behind Order 2!
    auto mod = book.modify(1, 100, 100, 3);
    EXPECT_EQ(mod.reject, te::RejectReason::None);

    // Incoming Sell of 60: Order 2 must fill 50 first!
    auto match = book.add(te::NewOrder{.id = 3, .price = 100, .timestamp = 4, .quantity = 60, .symbol = 1, .side = te::Side::Sell, .type = te::OrderType::Limit});
    ASSERT_EQ(match.trades.size(), 2);
    EXPECT_EQ(match.trades[0].buy_order_id, 2);  // Order 2 filled first!
    EXPECT_EQ(match.trades[0].quantity, 50);
    EXPECT_EQ(match.trades[1].buy_order_id, 1);  // Order 1 filled remaining 10
    EXPECT_EQ(match.trades[1].quantity, 10);

    EXPECT_EQ(book.depth_at(te::Side::Buy, 100), 90);
    EXPECT_NO_THROW(book.check_invariants());
}

TEST(CancelModifyTest, ModifyQuantityZeroCancels) {
    te::OrderBook book(1);

    static_cast<void>(book.add(te::NewOrder{.id = 1, .price = 100, .timestamp = 1, .quantity = 50, .symbol = 1, .side = te::Side::Buy, .type = te::OrderType::Limit}));
    EXPECT_EQ(book.order_count(), 1);

    auto res = book.modify(1, 100, 0);
    EXPECT_EQ(res.reject, te::RejectReason::None);
    EXPECT_EQ(book.order_count(), 0);
    EXPECT_EQ(book.best_bid(), std::nullopt);
    EXPECT_NO_THROW(book.check_invariants());
}
