#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <algorithm>

struct MarketTick {
    std::string timestamp;
    double price;
    double volume;
};

struct Order {
    std::string orderId;
    std::string type; // "BUY" or "SELL"
    double price;
    double quantity;
    bool isExecuted;
};

class IStrategyPlanner {
public:
    virtual ~IStrategyPlanner() = default;
    virtual std::string evaluateMarketState(const MarketTick& tick) = 0;
};

class MovingAverageCrossPlanner : public IStrategyPlanner {
private:
    std::vector<double> priceHistory;
    size_t fastWindow = 5;
    size_t slowWindow = 10;

    double calculateMA(size_t windowSize) {
        if (priceHistory.size() < windowSize) return 0.0;
        double sum = 0.0;
        for (size_t i = priceHistory.size() - windowSize; i < priceHistory.size(); ++i) {
            sum += priceHistory[i];
        }
        return sum / windowSize;
    }

public:
    std::string evaluateMarketState(const MarketTick& tick) override {
        priceHistory.push_back(tick.price);
        if (priceHistory.size() < slowWindow) return "HOLD";

        double fastMA = calculateMA(fastWindow);
        double slowMA = calculateMA(slowWindow);

        if (fastMA > slowMA) return "BUY";
        if (fastMA < slowMA) return "SELL";
        return "HOLD";
    }
};

class RiskGuardrail {
private:
    double accountBalance = 10000.0;

public:
    bool validateOrderSafety(const Order& order) {
        double prospectiveCost = order.price * order.quantity;
        if (prospectiveCost > accountBalance && order.type == "BUY") {
            std::cerr << "[CRITICAL RISK WARNING]: Order rejected due to capital limit violation.\n";
            return false;
        }
        if (order.price <= 0.0 || order.quantity <= 0.0) {
            std::cerr << "[CRITICAL RISK WARNING]: Invalid order metrics detected.\n";
            return false;
        }
        return true;
    }
};

class TradingSystemOrchestrator {
private:
    std::unique_ptr<IStrategyPlanner> strategyPlanner;
    RiskGuardrail riskGuardrail;
    std::vector<Order> orderBook;
    int orderCounter = 0;

public:
    TradingSystemOrchestrator(std::unique_ptr<IStrategyPlanner> planner) 
        : strategyPlanner(std::move(planner)) {}

    void processIncomingMarketEvent(const MarketTick& tick) {
        std::cout << "Processing Market Frame -> Price: " << tick.price << " | Vol: " << tick.volume << "\n";
        std::string action = strategyPlanner->evaluateMarketState(tick);
        
        if (action == "HOLD") return;

        orderCounter++;
        Order prospectiveOrder{"ORD_" + std::to_string(orderCounter), action, tick.price, 10.0, false};

        if (riskGuardrail.validateOrderSafety(prospectiveOrder)) {
            prospectiveOrder.isExecuted = true;
            orderBook.push_back(prospectiveOrder);
            std::cout << "[EXECUTION SUCCESS]: " << action << " order processed dynamically: " << prospectiveOrder.orderId << "\n";
        }
    }

    size_t getExecutedOrderCount() const { return orderBook.size(); }
};

int main() {
    std::cout << "=== Initializing AetherDrift Trading System Core Engine ===\n\n";
    auto strategy = std::make_unique<MovingAverageCrossPlanner>();
    TradingSystemOrchestrator engine(std::move(strategy));

    std::vector<MarketTick> simulatedFeed = {
        {"10:00", 100.0, 1500}, {"10:01", 101.5, 1200}, {"10:02", 102.0, 1100},
        {"10:03", 100.5, 1300}, {"10:04", 99.0,  1800}, {"10:05", 97.5,  2000},
        {"10:06", 96.0,  2500}, {"10:07", 98.0,  2200}, {"10:08", 100.2, 1900},
        {"10:09", 102.5, 2400}, {"10:10", 105.0, 3000}, {"10:11", 108.5, 3500}
    };

    for (const auto& tick : simulatedFeed) {
        engine.processIncomingMarketEvent(tick);
    }

    std::cout << "\nExecution terminated cleanly. Total Validated Events Logged: " << engine.getExecutedOrderCount() << "\n";
    return 0;
}