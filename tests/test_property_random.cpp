#include <gtest/gtest.h>

#include <random>
#include <vector>

#include "reference_book.hpp"
#include "te/order_book.hpp"

namespace {

struct FuzzAction {
    enum class Type { AddLimit, AddMarket, Cancel, Modify };
    Type type{Type::AddLimit};
    te::NewOrder order{};
    te::Price new_price{0};
    te::Quantity new_qty{0};
};

}  // namespace

TEST(PropertyTest, MatchesReferenceModelAcrossSeeds) {
    // Run across 50 distinct fixed seeds for reproducible, deterministic verification
    constexpr std::uint64_t NUM_SEEDS = 50;
    constexpr int OPS_PER_SEED = 1000;

    for (std::uint64_t seed = 1; seed <= NUM_SEEDS; ++seed) {
        std::mt19937_64 rng(seed);
        std::uniform_int_distribution<int> action_dist(0, 99);
        std::uniform_int_distribution<int> side_dist(0, 1);
        std::uniform_int_distribution<int> price_dist(90, 110);
        std::uniform_int_distribution<int> qty_dist(1, 100);

        te::OrderBook fast(1);
        te::test::ReferenceBook ref(1);

        std::vector<te::OrderId> active_ids;
        te::OrderId next_id = 1;
        te::Timestamp ts = 100;

        for (int step = 0; step < OPS_PER_SEED; ++step) {
            ts += 1;
            const int roll = action_dist(rng);

            if (roll < 65 || active_ids.empty()) {
                // 65% Add Limit
                const te::OrderId id = next_id++;
                active_ids.push_back(id);

                const te::NewOrder order{
                    .id = id,
                    .price = price_dist(rng),
                    .timestamp = ts,
                    .quantity = static_cast<te::Quantity>(qty_dist(rng)),
                    .symbol = 1,
                    .side = (side_dist(rng) == 0) ? te::Side::Buy : te::Side::Sell,
                    .type = te::OrderType::Limit
                };

                const auto fast_res = fast.add(order);
                const auto ref_res = ref.add(order);

                ASSERT_EQ(fast_res.reject, ref_res.reject)
                    << "Reject mismatch at seed=" << seed << " step=" << step;
                ASSERT_EQ(fast_res.unfilled, ref_res.unfilled)
                    << "Unfilled mismatch at seed=" << seed << " step=" << step;
                ASSERT_EQ(fast_res.trades.size(), ref_res.trades.size())
                    << "Trade count mismatch at seed=" << seed << " step=" << step;

                for (std::size_t t = 0; t < fast_res.trades.size(); ++t) {
                    EXPECT_EQ(fast_res.trades[t].price, ref_res.trades[t].price)
                        << "Trade price mismatch at seed=" << seed << " step=" << step;
                    EXPECT_EQ(fast_res.trades[t].quantity, ref_res.trades[t].quantity)
                        << "Trade qty mismatch at seed=" << seed << " step=" << step;
                    EXPECT_EQ(fast_res.trades[t].buy_order_id, ref_res.trades[t].buy_order_id);
                    EXPECT_EQ(fast_res.trades[t].sell_order_id, ref_res.trades[t].sell_order_id);
                }
            } else if (roll < 75) {
                // 10% Add Market Order
                const te::OrderId id = next_id++;
                const te::NewOrder order{
                    .id = id,
                    .price = 0,
                    .timestamp = ts,
                    .quantity = static_cast<te::Quantity>(qty_dist(rng)),
                    .symbol = 1,
                    .side = (side_dist(rng) == 0) ? te::Side::Buy : te::Side::Sell,
                    .type = te::OrderType::Market
                };

                const auto fast_res = fast.add(order);
                const auto ref_res = ref.add(order);

                ASSERT_EQ(fast_res.reject, ref_res.reject);
                ASSERT_EQ(fast_res.unfilled, ref_res.unfilled);
                ASSERT_EQ(fast_res.trades.size(), ref_res.trades.size());

                for (std::size_t t = 0; t < fast_res.trades.size(); ++t) {
                    EXPECT_EQ(fast_res.trades[t].price, ref_res.trades[t].price);
                    EXPECT_EQ(fast_res.trades[t].quantity, ref_res.trades[t].quantity);
                    EXPECT_EQ(fast_res.trades[t].buy_order_id, ref_res.trades[t].buy_order_id);
                    EXPECT_EQ(fast_res.trades[t].sell_order_id, ref_res.trades[t].sell_order_id);
                }
            } else if (roll < 90) {
                // 15% Cancel Order
                std::uniform_int_distribution<std::size_t> idx_dist(0, active_ids.size() - 1);
                const std::size_t pick = idx_dist(rng);
                const te::OrderId id_to_cancel = active_ids[pick];

                const auto fast_rej = fast.cancel(id_to_cancel);
                const auto ref_rej = ref.cancel(id_to_cancel);

                ASSERT_EQ(fast_rej, ref_rej)
                    << "Cancel reject mismatch at seed=" << seed << " step=" << step;
            } else {
                // 10% Modify Order
                std::uniform_int_distribution<std::size_t> idx_dist(0, active_ids.size() - 1);
                const std::size_t pick = idx_dist(rng);
                const te::OrderId id_to_mod = active_ids[pick];
                const te::Price new_p = price_dist(rng);
                const te::Quantity new_q = static_cast<te::Quantity>(qty_dist(rng));

                const auto fast_res = fast.modify(id_to_mod, new_p, new_q, ts);
                const auto ref_res = ref.modify(id_to_mod, new_p, new_q, ts);

                ASSERT_EQ(fast_res.reject, ref_res.reject);
                ASSERT_EQ(fast_res.trades.size(), ref_res.trades.size());
            }

            if (step % 100 == 0) {
                fast.check_invariants();
            }
        }

        fast.check_invariants();
    }
}
