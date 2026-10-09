#include <iostream>
#include <sstream>
#include <string>

#include "te/order_book.hpp"

int main() {
    te::OrderBook book(1);  // Symbol 1

    std::cout << "--- Order Book CLI (Symbol 1) ---\n";
    std::cout << "Commands:\n";
    std::cout << "  BUY L <price> <qty> <id>\n";
    std::cout << "  SELL L <price> <qty> <id>\n";
    std::cout << "  BUY M <qty> <id>\n";
    std::cout << "  SELL M <qty> <id>\n";
    std::cout << "  CANCEL <id>\n";
    std::cout << "  MODIFY <id> <new_price> <new_qty>\n";
    std::cout << "  PRINT\n";
    std::cout << "  QUIT\n\n";

    std::string line;
    while (std::cout << "> " && std::getline(std::cin, line)) {
        if (line.empty()) continue;

        std::istringstream iss(line);
        std::string action;
        iss >> action;

        if (action == "QUIT" || action == "EXIT" || action == "quit") {
            break;
        }

        if (action == "BUY" || action == "SELL") {
            const te::Side side = (action == "BUY") ? te::Side::Buy : te::Side::Sell;
            char type_char;
            iss >> type_char;

            te::NewOrder order;
            order.symbol = 1;
            order.side = side;

            if (type_char == 'L' || type_char == 'l') {
                order.type = te::OrderType::Limit;
                iss >> order.price >> order.quantity >> order.id;
            } else if (type_char == 'M' || type_char == 'm') {
                order.type = te::OrderType::Market;
                order.price = 0;
                iss >> order.quantity >> order.id;
            } else {
                std::cout << "ERROR: Unknown order type '" << type_char << "' (use L or M)\n";
                continue;
            }

            auto result = book.add(order);
            if (result.reject != te::RejectReason::None) {
                std::cout << "REJECT " << te::to_string(result.reject) << "\n";
            } else {
                for (const auto& tr : result.trades) {
                    std::cout << "TRADE id=" << tr.trade_id << " " << tr.quantity << " @ "
                              << tr.price << " buy=" << tr.buy_order_id << " sell=" << tr.sell_order_id << "\n";
                }
                if (result.unfilled > 0) {
                    std::cout << "UNFILLED " << result.unfilled << "\n";
                }
                if (result.trades.empty() && result.unfilled == 0) {
                    std::cout << "ACK " << order.id << "\n";
                }
            }
        } else if (action == "CANCEL") {
            te::OrderId id;
            iss >> id;
            auto reject = book.cancel(id);
            if (reject == te::RejectReason::None) {
                std::cout << "CANCELLED " << id << "\n";
            } else {
                std::cout << "REJECT " << te::to_string(reject) << "\n";
            }
        } else if (action == "MODIFY") {
            te::OrderId id;
            te::Price new_price;
            te::Quantity new_qty;
            iss >> id >> new_price >> new_qty;
            auto result = book.modify(id, new_price, new_qty);
            if (result.reject != te::RejectReason::None) {
                std::cout << "REJECT " << te::to_string(result.reject) << "\n";
            } else {
                for (const auto& tr : result.trades) {
                    std::cout << "TRADE id=" << tr.trade_id << " " << tr.quantity << " @ "
                              << tr.price << " buy=" << tr.buy_order_id << " sell=" << tr.sell_order_id << "\n";
                }
                std::cout << "MODIFIED " << id << "\n";
            }
        } else if (action == "PRINT") {
            auto bb = book.best_bid();
            auto ba = book.best_ask();
            std::cout << "BOOK: Best Bid = " << (bb ? std::to_string(*bb) : "None")
                      << " | Best Ask = " << (ba ? std::to_string(*ba) : "None")
                      << " | Total Orders = " << book.order_count() << "\n";
        } else {
            std::cout << "ERROR: Unknown command '" << action << "'\n";
        }
    }

    return 0;
}
