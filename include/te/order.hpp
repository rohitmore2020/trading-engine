#pragma once

#include <type_traits>

#include "te/types.hpp"

namespace te {

// Internal resting order in the order book.
// Arranged largest to smallest to minimize struct padding and guarantee <= 64 bytes.
struct Order {
    OrderId id{0};                      // 8 bytes (offset 0)
    Price price{0};                     // 8 bytes (offset 8)
    Timestamp timestamp{0};             // 8 bytes (offset 16)
    Quantity quantity{0};               // 4 bytes (offset 24)
    Quantity remaining{0};              // 4 bytes (offset 28)
    SymbolId symbol{0};                 // 2 bytes (offset 32)
    Side side{Side::Buy};               // 1 byte  (offset 34)
    OrderType type{OrderType::Limit};   // 1 byte  (offset 35)
    // 4 bytes tail padding inserted by compiler to satisfy 8-byte alignment -> Total 40 bytes.

    [[nodiscard]] constexpr bool is_filled() const noexcept {
        return remaining == 0;
    }

    [[nodiscard]] constexpr Quantity filled_quantity() const noexcept {
        return quantity - remaining;
    }
};

static_assert(std::is_trivially_copyable_v<Order>, "Order must be trivially copyable");
static_assert(sizeof(Order) <= 64, "Order must fit within a single 64-byte cache line");

}  // namespace te
