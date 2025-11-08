// include/api/RestApi.hpp
#pragma once

#include "../engine/MatchingEngine.hpp"
#include "../monitoring/Metrics.hpp"
#include "../risk/RiskEngine.hpp"
#include <cpprest/http_listener.h>
#include <cpprest/json.h>
#include <memory>
#include <thread>
#include <atomic>

namespace api {

using namespace web;
using namespace web::http;
using namespace web::http::experimental::listener;

class RestApi {
public:
    RestApi(const std::string& address, 
            std::shared_ptr<engine::MatchingEngine> engine,
            std::shared_ptr<monitoring::Metrics> metrics,
            std::shared_ptr<risk::RiskEngine> riskEngine);
    ~RestApi();
    
    void start();
    void stop();
    
private:
    http_listener listener_;
    std::shared_ptr<engine::MatchingEngine> engine_;
    std::shared_ptr<monitoring::Metrics> metrics_;
    std::shared_ptr<risk::RiskEngine> riskEngine_;
    std::atomic<bool> running_{false};
    std::thread serverThread_;
    
    // Route handlers
    void handleGet(const http_request& request);
    void handlePost(const http_request& request);
    void handlePut(const http_request& request);
    void handleDelete(const http_request& request);
    
    // Specific endpoints
    void handleHealth(const http_request& request);
    void handleStatistics(const http_request& request);
    void handleOrderBook(const http_request& request);
    void handleSubmitOrder(const http_request& request);
    void handleCancelOrder(const http_request& request);
    void handlePositions(const http_request& request);
    void handleRiskLimits(const http_request& request);
    void handleSystemStatus(const http_request& request);
    void handleConfig(const http_request& request);
    
    // Utility methods
    json::value engineStatusToJson(engine::EngineStatus status);
    json::value orderToJson(const engine::Order& order);
    json::value tradeToJson(const engine::Trade& trade);
    json::value positionToJson(const risk::Position& position);
    json::value marketDataToJson(const engine::OrderBook::Depth& depth);
    
    // Error handling
    void sendErrorResponse(const http_request& request, status_code code, const std::string& message);
};

} // namespace api