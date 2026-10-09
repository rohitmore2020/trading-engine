#include <gtest/gtest.h>

#include "te/validation.hpp"

TEST(ValidationTest, ValidLimitOrderPasses) {
    te::NewOrder order{
        .id = 1,
        .price = 10050,  // $100.50
        .timestamp = 100,
        .quantity = 50,
        .symbol = 1,
        .side = te::Side::Buy,
        .type = te::OrderType::Limit
    };

    EXPECT_EQ(te::validate(order), te::RejectReason::None);
}

TEST(ValidationTest, ValidMarketOrderWithZeroPricePasses) {
    te::NewOrder order{
        .id = 2,
        .price = 0,  // Market orders do not specify a limit price
        .timestamp = 200,
        .quantity = 25,
        .symbol = 1,
        .side = te::Side::Sell,
        .type = te::OrderType::Market
    };

    EXPECT_EQ(te::validate(order), te::RejectReason::None);
}

TEST(ValidationTest, RejectsZeroQuantity) {
    te::NewOrder order{
        .id = 3,
        .price = 5000,
        .timestamp = 300,
        .quantity = 0,
        .symbol = 1,
        .side = te::Side::Buy,
        .type = te::OrderType::Limit
    };

    EXPECT_EQ(te::validate(order), te::RejectReason::ZeroQuantity);
}

TEST(ValidationTest, RejectsZeroOrderId) {
    te::NewOrder order{
        .id = 0,
        .price = 5000,
        .timestamp = 300,
        .quantity = 10,
        .symbol = 1,
        .side = te::Side::Buy,
        .type = te::OrderType::Limit
    };

    EXPECT_EQ(te::validate(order), te::RejectReason::UnknownOrder);
}

TEST(ValidationTest, RejectsZeroSymbolId) {
    te::NewOrder order{
        .id = 4,
        .price = 5000,
        .timestamp = 300,
        .quantity = 10,
        .symbol = 0,
        .side = te::Side::Buy,
        .type = te::OrderType::Limit
    };

    EXPECT_EQ(te::validate(order), te::RejectReason::UnknownSymbol);
}

TEST(ValidationTest, RejectsNonPositivePriceForLimitOrders) {
    te::NewOrder zero_price{
        .id = 5,
        .price = 0,
        .timestamp = 400,
        .quantity = 10,
        .symbol = 1,
        .side = te::Side::Buy,
        .type = te::OrderType::Limit
    };
    EXPECT_EQ(te::validate(zero_price), te::RejectReason::NonPositivePrice);

    te::NewOrder negative_price{
        .id = 6,
        .price = -100,
        .timestamp = 400,
        .quantity = 10,
        .symbol = 1,
        .side = te::Side::Buy,
        .type = te::OrderType::Limit
    };
    EXPECT_EQ(te::validate(negative_price), te::RejectReason::NonPositivePrice);
}

TEST(ValidationTest, RejectReasonToString) {
    EXPECT_EQ(te::to_string(te::RejectReason::None), "None");
    EXPECT_EQ(te::to_string(te::RejectReason::ZeroQuantity), "ZeroQuantity");
    EXPECT_EQ(te::to_string(te::RejectReason::NonPositivePrice), "NonPositivePrice");
    EXPECT_EQ(te::to_string(te::RejectReason::DuplicateId), "DuplicateId");
    EXPECT_EQ(te::to_string(te::RejectReason::UnknownOrder), "UnknownOrder");
    EXPECT_EQ(te::to_string(te::RejectReason::UnknownSymbol), "UnknownSymbol");
    EXPECT_EQ(te::to_string(te::RejectReason::PriceOutOfRange), "PriceOutOfRange");
}
