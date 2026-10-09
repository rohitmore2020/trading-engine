#pragma once

#include <cstdint>
#include <type_traits>

#include "te/types.hpp"

namespace te {

// Client requests
struct NewOrder {
    OrderId id{0};
    Price price{0};                     // Ticks (ignored for Market orders)
    Timestamp timestamp{0};
    Quantity quantity{0};
    SymbolId symbol{0};
    Side side{Side::Buy};
    OrderType type{OrderType::Limit};

    [[nodiscard]] constexpr bool operator==(const NewOrder& other) const noexcept = default;
};

struct CancelOrder {
    OrderId id{0};
    Timestamp timestamp{0};
    SymbolId symbol{0};

    [[nodiscard]] constexpr bool operator==(const CancelOrder& other) const noexcept = default;
};

struct ModifyOrder {
    OrderId id{0};
    Price new_price{0};
    Timestamp timestamp{0};
    Quantity new_quantity{0};
    SymbolId symbol{0};

    [[nodiscard]] constexpr bool operator==(const ModifyOrder& other) const noexcept = default;
};

enum class CommandType : std::uint8_t {
    New = 0,
    Cancel = 1,
    Modify = 2
};

// Flat, trivially-copyable Command struct suitable for lock-free queues
struct Command {
    CommandType type{CommandType::New};
    NewOrder order{};

    // Helper constructors
    [[nodiscard]] static constexpr Command make_new(const NewOrder& o) noexcept {
        Command cmd;
        cmd.type = CommandType::New;
        cmd.order = o;
        return cmd;
    }

    [[nodiscard]] static constexpr Command make_cancel(OrderId id, SymbolId symbol, Timestamp ts = 0) noexcept {
        Command cmd;
        cmd.type = CommandType::Cancel;
        cmd.order.id = id;
        cmd.order.symbol = symbol;
        cmd.order.timestamp = ts;
        return cmd;
    }

    [[nodiscard]] static constexpr Command make_modify(OrderId id, SymbolId symbol, Price new_price, Quantity new_qty, Timestamp ts = 0) noexcept {
        Command cmd;
        cmd.type = CommandType::Modify;
        cmd.order.id = id;
        cmd.order.symbol = symbol;
        cmd.order.price = new_price;
        cmd.order.quantity = new_qty;
        cmd.order.timestamp = ts;
        return cmd;
    }
};

static_assert(std::is_trivially_copyable_v<NewOrder>, "NewOrder must be trivially copyable");
static_assert(std::is_trivially_copyable_v<CancelOrder>, "CancelOrder must be trivially copyable");
static_assert(std::is_trivially_copyable_v<ModifyOrder>, "ModifyOrder must be trivially copyable");
static_assert(std::is_trivially_copyable_v<Command>, "Command must be trivially copyable");
static_assert(sizeof(Command) <= 64, "Command must fit within a 64-byte cache line");

}  // namespace te
