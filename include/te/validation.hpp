#pragma once

#include "te/command.hpp"
#include "te/types.hpp"

namespace te {

[[nodiscard]] constexpr RejectReason validate(const NewOrder& order) noexcept {
    if (order.quantity == 0) {
        return RejectReason::ZeroQuantity;
    }

    if (order.id == 0) {
        return RejectReason::UnknownOrder;
    }

    if (order.symbol == 0) {
        return RejectReason::UnknownSymbol;
    }

    if (order.type == OrderType::Limit && order.price <= 0) {
        return RejectReason::NonPositivePrice;
    }

    return RejectReason::None;
}

}  // namespace te
