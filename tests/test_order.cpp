#include <gtest/gtest.h>

#include "te/command.hpp"
#include "te/order.hpp"
#include "te/trade.hpp"
#include "te/types.hpp"

TEST(OrderDomainTest, StructSizesWithinCacheLine) {
    // A cache line is 64 bytes. All high-frequency domain objects should fit inside.
    EXPECT_LE(sizeof(te::Order), 64);
    EXPECT_LE(sizeof(te::Trade), 64);
    EXPECT_LE(sizeof(te::Command), 64);

    std::cout << "[INFO] sizeof(Order)   = " << sizeof(te::Order) << " bytes\n";
    std::cout << "[INFO] sizeof(Trade)   = " << sizeof(te::Trade) << " bytes\n";
    std::cout << "[INFO] sizeof(Command) = " << sizeof(te::Command) << " bytes\n";
}

TEST(OrderDomainTest, TriviallyCopyableRequirements) {
    EXPECT_TRUE(std::is_trivially_copyable_v<te::Order>);
    EXPECT_TRUE(std::is_trivially_copyable_v<te::Trade>);
    EXPECT_TRUE(std::is_trivially_copyable_v<te::NewOrder>);
    EXPECT_TRUE(std::is_trivially_copyable_v<te::CancelOrder>);
    EXPECT_TRUE(std::is_trivially_copyable_v<te::ModifyOrder>);
    EXPECT_TRUE(std::is_trivially_copyable_v<te::Command>);
}

TEST(OrderDomainTest, OrderStateCalculations) {
    te::Order order{
        .id = 101,
        .price = 15000,  // $150.00
        .timestamp = 1000,
        .quantity = 100,
        .remaining = 100,
        .symbol = 1,
        .side = te::Side::Buy,
        .type = te::OrderType::Limit
    };

    EXPECT_FALSE(order.is_filled());
    EXPECT_EQ(order.filled_quantity(), 0);

    order.remaining = 40;
    EXPECT_FALSE(order.is_filled());
    EXPECT_EQ(order.filled_quantity(), 60);

    order.remaining = 0;
    EXPECT_TRUE(order.is_filled());
    EXPECT_EQ(order.filled_quantity(), 100);
}

TEST(OrderDomainTest, SideAndTypeUtilities) {
    EXPECT_TRUE(te::is_buy(te::Side::Buy));
    EXPECT_FALSE(te::is_buy(te::Side::Sell));

    EXPECT_EQ(te::opposite_side(te::Side::Buy), te::Side::Sell);
    EXPECT_EQ(te::opposite_side(te::Side::Sell), te::Side::Buy);

    EXPECT_EQ(te::to_string(te::Side::Buy), "BUY");
    EXPECT_EQ(te::to_string(te::Side::Sell), "SELL");

    EXPECT_EQ(te::to_string(te::OrderType::Limit), "LIMIT");
    EXPECT_EQ(te::to_string(te::OrderType::Market), "MARKET");
}
