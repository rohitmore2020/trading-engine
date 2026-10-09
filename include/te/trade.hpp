#pragma once

#include <cstdint>
#include <type_traits>

#include "te/types.hpp"

namespace te {

// Emitted when two orders match.
// Trivially copyable and <= 64 bytes.
struct Trade {
    std::uint64_t trade_id{0};          // 8 bytes (offset 0)
    OrderId buy_order_id{0};            // 8 bytes (offset 8)
    OrderId sell_order_id{0};           // 8 bytes (offset 16)
    Price price{0};                     // 8 bytes (offset 24)
    Timestamp timestamp{0};             // 8 bytes (offset 32)
    Quantity quantity{0};               // 4 bytes (offset 40)
    SymbolId symbol{0};                 // 2 bytes (offset 44)
    // 2 bytes tail padding -> Total 48 bytes.

    [[nodiscard]] constexpr bool operator==(const Trade& other) const noexcept = default;
};

static_assert(std::is_trivially_copyable_v<Trade>, "Trade must be trivially copyable");
static_assert(sizeof(Trade) <= 64, "Trade must fit within a single 64-byte cache line");

}  // namespace te
