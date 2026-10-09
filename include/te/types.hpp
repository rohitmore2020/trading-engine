#pragma once

#include <cstdint>
#include <string_view>

namespace te {

// Core numeric aliases
using OrderId = std::uint64_t;
using Price = std::int64_t;        // Represented in ticks (integer arithmetic only)
using Quantity = std::uint32_t;
using SymbolId = std::uint16_t;
using Timestamp = std::int64_t;    // Nanoseconds since epoch

// Scoped 1-byte enums
enum class Side : std::uint8_t {
    Buy = 0,
    Sell = 1
};

enum class OrderType : std::uint8_t {
    Limit = 0,
    Market = 1
};

enum class RejectReason : std::uint8_t {
    None = 0,
    ZeroQuantity,
    NonPositivePrice,
    DuplicateId,
    UnknownOrder,
    UnknownSymbol,
    PriceOutOfRange
};

// Compile-time helper utilities
[[nodiscard]] constexpr bool is_buy(Side side) noexcept {
    return side == Side::Buy;
}

[[nodiscard]] constexpr Side opposite_side(Side side) noexcept {
    return side == Side::Buy ? Side::Sell : Side::Buy;
}

[[nodiscard]] constexpr std::string_view to_string(Side side) noexcept {
    return side == Side::Buy ? "BUY" : "SELL";
}

[[nodiscard]] constexpr std::string_view to_string(OrderType type) noexcept {
    return type == OrderType::Limit ? "LIMIT" : "MARKET";
}

[[nodiscard]] constexpr std::string_view to_string(RejectReason reason) noexcept {
    switch (reason) {
        case RejectReason::None:
            return "None";
        case RejectReason::ZeroQuantity:
            return "ZeroQuantity";
        case RejectReason::NonPositivePrice:
            return "NonPositivePrice";
        case RejectReason::DuplicateId:
            return "DuplicateId";
        case RejectReason::UnknownOrder:
            return "UnknownOrder";
        case RejectReason::UnknownSymbol:
            return "UnknownSymbol";
        case RejectReason::PriceOutOfRange:
            return "PriceOutOfRange";
    }
    return "Unknown";
}

}  // namespace te
