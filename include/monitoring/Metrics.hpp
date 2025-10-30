// include/monitoring/Metrics.hpp
#pragma once

#include "../engine/Types.hpp"
#include "../utils/Clock.hpp"
#include <prometheus/counter.h>
#include <prometheus/gauge.h>
#include <prometheus/histogram.h>
#include <prometheus/registry.h>
#include <prometheus/exposer.h>
#include <memory>
#include <string>
#include <unordered_map>

namespace monitoring {

class Metrics {
public:
    Metrics(const utils::Config& config);
    ~Metrics();
    
    void recordOrder(const engine::Order& order, const OrderResponse& response);
    void recordTrade(const engine::Trade& trade);
    void recordLatency(uint64_t nanoseconds);
    void recordQueueSize(size_t size);
    
    void setEngineStatus(engine::EngineStatus status);
    void setConnectionStatus(bool connected);
    
    // Expose metrics for Prometheus
    void startExposer(const std::string& endpoint = "0.0.0.0:8080");
    
private:
    std::shared_ptr<prometheus::Registry> registry_;
    std::unique_ptr<prometheus::Exposer> exposer_;
    utils::NanosecondClock clock_;
    
    // Counters
    prometheus::Counter& ordersTotal_;
    prometheus::Counter& tradesTotal_;
    prometheus::Counter& orderVolume_;
    prometheus::Counter& tradeVolume_;
    
    // Gauges
    prometheus::Gauge& orderBookDepth_;
    prometheus::Gauge& queueSize_;
    prometheus::Gauge& engineStatus_;
    prometheus::Gauge& connectionStatus_;
    
    // Histograms
    prometheus::Histogram& orderLatency_;
    prometheus::Histogram& tradeLatency_;
    
    // Per-instrument metrics
    std::unordered_map<std::string, prometheus::Counter*> instrumentOrders_;
    std::unordered_map<std::string, prometheus::Counter*> instrumentTrades_;
    std::unordered_map<std::string, prometheus::Counter*> instrumentVolume_;
    
    mutable std::shared_mutex metricsMutex_;
    
    void ensureInstrumentMetrics(const std::string& symbol);
};

} // namespace monitoring