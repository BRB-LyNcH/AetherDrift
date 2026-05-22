#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

struct MarketTick {
    std::string timestamp;
    double price;
    double volume;
};

enum class OrderType {
    NONE,
    BUY,
    SELL
};

static std::string orderTypeToString(OrderType type) {
    switch (type) {
        case OrderType::BUY:
            return "BUY";
        case OrderType::SELL:
            return "SELL";
        default:
            return "HOLD";
    }
}

struct TradeSignal {
    OrderType type = OrderType::NONE;
    double confidence = 0.0;
};

struct Order {
    std::string orderId;
    OrderType type;
    double price;
    double quantity;
    bool isExecuted;
};

class IStrategyPlanner {
public:
    virtual ~IStrategyPlanner() = default;
    virtual TradeSignal evaluateMarketState(const MarketTick& tick, double currentPosition) = 0;
};

class MovingAverageCrossPlanner : public IStrategyPlanner {
private:
    std::vector<double> priceHistory;
    std::vector<double> volumeHistory;
    size_t fastWindow = 5;
    size_t slowWindow = 12;
    double momentumThreshold = 0.003;
    double volumeMultiplier = 0.85;
    double lastVolatility = 0.01;

    double calculateEMA(const std::vector<double>& values, size_t windowSize) const {
        if (values.size() < windowSize) {
            return 0.0;
        }

        double alpha = 2.0 / (windowSize + 1.0);
        double ema = std::accumulate(values.end() - windowSize, values.end(), 0.0) / static_cast<double>(windowSize);
        for (size_t idx = values.size() - windowSize + 1; idx < values.size(); ++idx) {
            ema = alpha * values[idx] + (1.0 - alpha) * ema;
        }
        return ema;
    }

    double calculateVolatility(size_t windowSize) const {
        if (priceHistory.size() < windowSize + 1) {
            return 0.01;
        }

        std::vector<double> returns;
        returns.reserve(windowSize);
        for (size_t i = priceHistory.size() - windowSize; i < priceHistory.size(); ++i) {
            double prevPrice = priceHistory[i - 1];
            if (prevPrice == 0.0) {
                continue;
            }
            returns.push_back((priceHistory[i] - prevPrice) / prevPrice);
        }

        if (returns.empty()) {
            return 0.01;
        }

        double mean = std::accumulate(returns.begin(), returns.end(), 0.0) / static_cast<double>(returns.size());
        double variance = 0.0;
        for (double r : returns) {
            variance += (r - mean) * (r - mean);
        }
        variance /= static_cast<double>(returns.size());
        return std::sqrt(variance);
    }

    double averageVolume(size_t windowSize) const {
        if (volumeHistory.size() < windowSize) {
            if (volumeHistory.empty()) {
                return 0.0;
            }
            return std::accumulate(volumeHistory.begin(), volumeHistory.end(), 0.0) / static_cast<double>(volumeHistory.size());
        }
        return std::accumulate(volumeHistory.end() - windowSize, volumeHistory.end(), 0.0) / static_cast<double>(windowSize);
    }

public:
    TradeSignal evaluateMarketState(const MarketTick& tick, double /*currentPosition*/) override {
        priceHistory.push_back(tick.price);
        volumeHistory.push_back(tick.volume);

        if (priceHistory.size() < slowWindow + 1) {
            lastVolatility = 0.01;
            return TradeSignal{OrderType::NONE, 0.0};
        }

        double fastEMA = calculateEMA(priceHistory, fastWindow);
        double slowEMA = calculateEMA(priceHistory, slowWindow);
        double diff = fastEMA - slowEMA;
        double normalizedDiff = slowEMA != 0.0 ? diff / slowEMA : 0.0;
        lastVolatility = std::max(calculateVolatility(slowWindow), 0.001);
        double volumeAvg = averageVolume(slowWindow);
        bool volumeConfirm = tick.volume >= volumeAvg * volumeMultiplier;

        if (normalizedDiff > momentumThreshold && volumeConfirm) {
            double confidence = std::min(1.0, normalizedDiff / (momentumThreshold * 3.0));
            return TradeSignal{OrderType::BUY, confidence};
        }

        if (normalizedDiff < -momentumThreshold && volumeConfirm) {
            double confidence = std::min(1.0, -normalizedDiff / (momentumThreshold * 3.0));
            return TradeSignal{OrderType::SELL, confidence};
        }

        return TradeSignal{OrderType::NONE, 0.0};
    }

    double getLastVolatility() const {
        return lastVolatility;
    }
};

class RiskGuardrail {
private:
    double accountBalance = 10000.0;
    double maxExposurePercent = 0.20;
    double maxRiskPerTrade = 0.01;
    double minimumQuantity = 1.0;

public:
    double getAccountBalance() const {
        return accountBalance;
    }

    bool validateOrderSafety(const Order& order, double currentExposure, double currentPosition) const {
        double futurePosition = currentPosition + (order.type == OrderType::BUY ? order.quantity : -order.quantity);
        double futureExposure = std::abs(futurePosition * order.price);
        double maxExposureValue = accountBalance * maxExposurePercent;

        if (order.price <= 0.0 || order.quantity <= 0.0) {
            std::cerr << "[RISK REJECTED]: invalid execution metrics.\n";
            return false;
        }

        if (order.type == OrderType::BUY && order.quantity * order.price > accountBalance) {
            std::cerr << "[RISK REJECTED]: insufficient cash for BUY order.\n";
            return false;
        }

        if (futureExposure > maxExposureValue) {
            std::cerr << "[RISK REJECTED]: exposure cap exceeded.\n";
            return false;
        }

        return true;
    }

    double calculateOrderQuantity(double marketPrice, double marketVolatility) const {
        double riskAmount = accountBalance * maxRiskPerTrade;
        double unitRisk = std::max(marketVolatility * marketPrice, 0.5);
        double size = riskAmount / unitRisk;
        if (size < minimumQuantity) {
            size = minimumQuantity;
        }
        double maxSize = (accountBalance * maxExposurePercent) / marketPrice;
        return std::floor(std::min(size, maxSize));
    }

    void settleOrder(const Order& order, double& cash) const {
        double cost = order.price * order.quantity;
        if (order.type == OrderType::BUY) {
            cash -= cost;
        } else if (order.type == OrderType::SELL) {
            cash += cost;
        }
    }
};

class PositionManager {
private:
    double size = 0.0;
    double avgEntryPrice = 0.0;
    double realizedPnL = 0.0;

public:
    double getSize() const {
        return size;
    }

    double getAvgEntryPrice() const {
        return avgEntryPrice;
    }

    double getRealizedPnL() const {
        return realizedPnL;
    }

    double getUnrealizedPnL(double marketPrice) const {
        if (size == 0.0) {
            return 0.0;
        }
        if (size > 0.0) {
            return (marketPrice - avgEntryPrice) * size;
        }
        return (avgEntryPrice - marketPrice) * std::abs(size);
    }

    void applyExecutedOrder(const Order& order) {
        double quantity = order.quantity;
        if (order.type == OrderType::BUY) {
            if (size >= 0.0) {
                avgEntryPrice = (avgEntryPrice * size + order.price * quantity) / (size + quantity);
                size += quantity;
            } else {
                double closingQty = std::min(quantity, std::abs(size));
                realizedPnL += (avgEntryPrice - order.price) * closingQty;
                size += closingQty;
                double remainingQty = quantity - closingQty;
                if (remainingQty > 0.0) {
                    avgEntryPrice = order.price;
                    size = remainingQty;
                }
                if (size == 0.0) {
                    avgEntryPrice = 0.0;
                }
            }
        } else if (order.type == OrderType::SELL) {
            if (size <= 0.0) {
                double currentShortSize = std::abs(size);
                avgEntryPrice = (avgEntryPrice * currentShortSize + order.price * quantity) / (currentShortSize + quantity);
                size -= quantity;
            } else {
                double closingQty = std::min(quantity, size);
                realizedPnL += (order.price - avgEntryPrice) * closingQty;
                size -= closingQty;
                double remainingQty = quantity - closingQty;
                if (remainingQty > 0.0) {
                    avgEntryPrice = order.price;
                    size = -remainingQty;
                }
                if (size == 0.0) {
                    avgEntryPrice = 0.0;
                }
            }
        }
    }
};

class TradingSystemOrchestrator {
private:
    std::unique_ptr<IStrategyPlanner> strategyPlanner;
    RiskGuardrail riskGuardrail;
    PositionManager positionManager;
    std::vector<Order> orderBook;
    int orderCounter = 0;
    double cash = 10000.0;
    double peakEquity = 10000.0;
    double maxDrawdown = 0.0;
    double stopLossPct = 0.015;
    double takeProfitPct = 0.03;
    double lastMarketPrice = 0.0;

public:
    TradingSystemOrchestrator(std::unique_ptr<IStrategyPlanner> planner)
        : strategyPlanner(std::move(planner)) {}

    bool shouldExitPosition(double marketPrice, OrderType& exitSide, double& quantity) const {
        double positionSize = positionManager.getSize();
        if (positionSize == 0.0) {
            return false;
        }

        double avgPrice = positionManager.getAvgEntryPrice();
        if (positionSize > 0.0) {
            double stopPrice = avgPrice * (1.0 - stopLossPct);
            double takePrice = avgPrice * (1.0 + takeProfitPct);
            if (marketPrice <= stopPrice || marketPrice >= takePrice) {
                exitSide = OrderType::SELL;
                quantity = positionSize;
                return true;
            }
        } else {
            double stopPrice = avgPrice * (1.0 + stopLossPct);
            double takePrice = avgPrice * (1.0 - takeProfitPct);
            if (marketPrice >= stopPrice || marketPrice <= takePrice) {
                exitSide = OrderType::BUY;
                quantity = std::abs(positionSize);
                return true;
            }
        }
        return false;
    }

    double getCurrentExposure(double marketPrice) const {
        return std::abs(positionManager.getSize() * marketPrice);
    }

    void updatePerformance(double marketPrice) {
        lastMarketPrice = marketPrice;
        double equity = cash + positionManager.getUnrealizedPnL(marketPrice);
        if (equity > peakEquity) {
            peakEquity = equity;
        }
        double drawdown = peakEquity - equity;
        if (drawdown > maxDrawdown) {
            maxDrawdown = drawdown;
        }
    }

    void processIncomingMarketEvent(const MarketTick& tick) {
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "Processing tick " << tick.timestamp << " -> Price: " << tick.price << " | Vol: " << tick.volume << "\n";

        OrderType exitSide = OrderType::NONE;
        double exitQuantity = 0.0;
        bool exitTriggered = shouldExitPosition(tick.price, exitSide, exitQuantity);

        if (exitTriggered) {
            executeOrder(exitSide, tick.price, exitQuantity, "STOP/TP");
        }

        TradeSignal signal = strategyPlanner->evaluateMarketState(tick, positionManager.getSize());
        if (signal.type == OrderType::NONE) {
            std::cout << "  -> No entry signal. Holding current positions.\n";
            updatePerformance(tick.price);
            return;
        }

        double quantity = riskGuardrail.calculateOrderQuantity(tick.price, static_cast<MovingAverageCrossPlanner*>(strategyPlanner.get())->getLastVolatility());
        if (quantity <= 0.0) {
            std::cout << "  -> Risk sizing produced zero quantity. Skipping trade.\n";
            updatePerformance(tick.price);
            return;
        }

        executeOrder(signal.type, tick.price, quantity, "STRATEGY");
        updatePerformance(tick.price);
    }

    void executeOrder(OrderType type, double price, double quantity, const std::string& reason) {
        if (quantity <= 0.0) {
            return;
        }

        Order order{"ORD_" + std::to_string(++orderCounter), type, price, quantity, false};
        double currentExposure = getCurrentExposure(price);
        if (!riskGuardrail.validateOrderSafety(order, currentExposure, positionManager.getSize())) {
            return;
        }

        riskGuardrail.settleOrder(order, cash);
        positionManager.applyExecutedOrder(order);
        order.isExecuted = true;
        orderBook.push_back(order);

        std::cout << "[EXECUTE] " << reason << ": " << orderTypeToString(type)
                  << " " << order.orderId
                  << " | Qty: " << order.quantity
                  << " | Price: " << order.price
                  << " | Position: " << positionManager.getSize()
                  << " | AvgEntry: " << positionManager.getAvgEntryPrice()
                  << " | Cash: " << cash << "\n";
    }

    void summarizeBacktest() const {
        double finalEquity = cash + positionManager.getUnrealizedPnL(lastMarketPrice);
        double netReturn = (finalEquity - 10000.0) / 10000.0 * 100.0;

        std::cout << "\n=== Backtest Summary ===\n";
        std::cout << "Executed Orders: " << orderBook.size() << "\n";
        std::cout << "Realized PnL: " << positionManager.getRealizedPnL() << "\n";
        std::cout << "Unrealized PnL: " << positionManager.getUnrealizedPnL(lastMarketPrice) << "\n";
        std::cout << "Final Equity: " << finalEquity << "\n";
        std::cout << "Net Return: " << netReturn << "%\n";
        std::cout << "Max Drawdown: " << maxDrawdown << "\n";
    }
};

std::vector<MarketTick> loadMarketFeed(const std::string& path) {
    std::vector<MarketTick> feed;
    std::ifstream input(path);
    if (!input.is_open()) {
        return feed;
    }

    std::string header;
    std::getline(input, header);
    std::string line;

    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }

        std::stringstream ss(line);
        std::string timestamp;
        std::string priceText;
        std::string volumeText;

        std::getline(ss, timestamp, ',');
        std::getline(ss, priceText, ',');
        std::getline(ss, volumeText, ',');

        try {
            double price = std::stod(priceText);
            double volume = volumeText.empty() ? 0.0 : std::stod(volumeText);
            feed.push_back(MarketTick{timestamp, price, volume});
        } catch (...) {
            continue;
        }
    }

    return feed;
}

std::vector<MarketTick> makeSampleFeed() {
    return {
        {"10:00", 100.0, 1500}, {"10:01", 101.5, 1200}, {"10:02", 102.0, 1100},
        {"10:03", 100.5, 1300}, {"10:04", 99.0, 1800}, {"10:05", 97.5, 2000},
        {"10:06", 96.0, 2500}, {"10:07", 98.0, 2200}, {"10:08", 100.2, 1900},
        {"10:09", 102.5, 2400}, {"10:10", 105.0, 3000}, {"10:11", 108.5, 3500}
    };
}

int main(int argc, char* argv[]) {
    std::cout << "=== Initializing AetherDrift Trading System Core Engine ===\n\n";
    std::string csvPath;
    if (argc > 1) {
        csvPath = argv[1];
    }

    std::vector<MarketTick> feed = csvPath.empty() ? makeSampleFeed() : loadMarketFeed(csvPath);
    if (feed.empty()) {
        std::cout << "[WARNING] Failed to parse CSV feed or file was empty. Falling back to sample data.\n";
        feed = makeSampleFeed();
    }

    auto strategy = std::make_unique<MovingAverageCrossPlanner>();
    TradingSystemOrchestrator engine(std::move(strategy));

    for (const auto& tick : feed) {
        engine.processIncomingMarketEvent(tick);
    }

    engine.summarizeBacktest();
    return 0;
}
