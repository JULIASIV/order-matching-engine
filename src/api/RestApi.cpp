// src/api/RestApi.cpp
#include "RestApi.hpp"
#include "../utils/Logger.hpp"
#include <cpprest/http_listener.h>
#include <cpprest/json.h>

namespace api {

RestApi::RestApi(const std::string& address, 
                 std::shared_ptr<engine::MatchingEngine> engine,
                 std::shared_ptr<monitoring::Metrics> metrics,
                 std::shared_ptr<risk::RiskEngine> riskEngine)
    : listener_(address)
    , engine_(engine)
    , metrics_(metrics)
    , riskEngine_(riskEngine)
{
    // Setup request handlers
    listener_.support(methods::GET, std::bind(&RestApi::handleGet, this, std::placeholders::_1));
    listener_.support(methods::POST, std::bind(&RestApi::handlePost, this, std::placeholders::_1));
    listener_.support(methods::PUT, std::bind(&RestApi::handlePut, this, std::placeholders::_1));
    listener_.support(methods::DEL, std::bind(&RestApi::handleDelete, this, std::placeholders::_1));
    
    LOG_INFO("REST API initialized on {}", address);
}

RestApi::~RestApi() {
    stop();
}

void RestApi::start() {
    if (running_.exchange(true)) {
        return;
    }
    
    try {
        listener_.open()
            .then([this]() {
                LOG_INFO("REST API server started successfully");
            })
            .wait();
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to start REST API: {}", e.what());
        throw;
    }
}

void RestApi::stop() {
    if (!running_.exchange(false)) {
        return;
    }
    
    listener_.close()
        .then([this]() {
            LOG_INFO("REST API server stopped");
        })
        .wait();
}

void RestApi::handleGet(const http_request& request) {
    auto path = request.relative_uri().path();
    
    try {
        if (path == "/health") {
            handleHealth(request);
        } else if (path == "/statistics") {
            handleStatistics(request);
        } else if (path.find("/orderbook/") == 0) {
            handleOrderBook(request);
        } else if (path == "/positions") {
            handlePositions(request);
        } else if (path == "/system/status") {
            handleSystemStatus(request);
        } else if (path == "/config") {
            handleConfig(request);
        } else {
            sendErrorResponse(request, status_codes::NotFound, "Endpoint not found");
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Error handling GET request: {}", e.what());
        sendErrorResponse(request, status_codes::InternalError, "Internal server error");
    }
}

void RestApi::handlePost(const http_request& request) {
    auto path = request.relative_uri().path();
    
    try {
        if (path == "/orders") {
            handleSubmitOrder(request);
        } else {
            sendErrorResponse(request, status_codes::NotFound, "Endpoint not found");
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Error handling POST request: {}", e.what());
        sendErrorResponse(request, status_codes::InternalError, "Internal server error");
    }
}

void RestApi::handleHealth(const http_request& request) {
    json::value response;
    response[U("status")] = json::value::string(U("healthy"));
    response[U("timestamp")] = json::value::string(U("2024-01-01T00:00:00Z")); // Current time
    response[U("version")] = json::value::string(U("1.0.0"));
    
    request.reply(status_codes::OK, response);
}

void RestApi::handleStatistics(const http_request& request) {
    auto stats = engine_->getStatistics();
    
    json::value response;
    response[U("orders_processed")] = json::value::number(stats.ordersProcessed);
    response[U("trades_executed")] = json::value::number(stats.tradesExecuted);
    response[U("total_volume")] = json::value::number(stats.totalVolume);
    response[U("average_latency_ns")] = json::value::number(stats.avgLatencyNs);
    response[U("max_latency_ns")] = json::value::number(stats.maxLatencyNs);
    
    request.reply(status_codes::OK, response);
}

void RestApi::handleOrderBook(const http_request& request) {
    auto path = request.relative_uri().path();
    auto symbol = path.substr(std::string("/orderbook/").length());
    
    if (symbol.empty()) {
        sendErrorResponse(request, status_codes::BadRequest, "Symbol parameter required");
        return;
    }
    
    // Get depth parameter
    auto query = uri::split_query(request.relative_uri().query());
    int depth = 10;
    if (query.find(U("depth")) != query.end()) {
        depth = std::stoi(query[U("depth")]);
    }
    
    auto marketData = engine_->getMarketData(symbol, depth);
    
    json::value response;
    response[U("symbol")] = json::value::string(utility::conversions::to_string_t(symbol));
    response[U("timestamp")] = json::value::string(U("2024-01-01T00:00:00Z"));
    
    // Add bids
    json::value bids = json::value::array();
    for (size_t i = 0; i < marketData.bids.size(); ++i) {
        json::value level;
        level[U("price")] = json::value::number(marketData.bids[i].price);
        level[U("quantity")] = json::value::number(marketData.bids[i].totalQuantity);
        level[U("order_count")] = json::value::number(marketData.bids[i].orderCount);
        bids[i] = level;
    }
    response[U("bids")] = bids;
    
    // Add asks
    json::value asks = json::value::array();
    for (size_t i = 0; i < marketData.asks.size(); ++i) {
        json::value level;
        level[U("price")] = json::value::number(marketData.asks[i].price);
        level[U("quantity")] = json::value::number(marketData.asks[i].totalQuantity);
        level[U("order_count")] = json::value::number(marketData.asks[i].orderCount);
        asks[i] = level;
    }
    response[U("asks")] = asks;
    
    request.reply(status_codes::OK, response);
}

void RestApi::handleSubmitOrder(const http_request& request) {
    request.extract_json()
        .then([this, request](json::value body) {
            try {
                // Parse order from JSON
                auto typeStr = body[U("type")].as_string();
                auto sideStr = body[U("side")].as_string();
                auto symbol = body[U("symbol")].as_string();
                auto price = body[U("price")].as_double();
                auto quantity = body[U("quantity")].as_number().to_int64();
                
                // Convert string to enum
                engine::OrderType type;
                if (typeStr == U("limit")) type = engine::OrderType::LIMIT;
                else if (typeStr == U("market")) type = engine::OrderType::MARKET;
                else if (typeStr == U("fok")) type = engine::OrderType::FOK;
                else if (typeStr == U("ioc")) type = engine::OrderType::IOC;
                else throw std::runtime_error("Invalid order type");
                
                engine::OrderSide side;
                if (sideStr == U("buy")) side = engine::OrderSide::BUY;
                else if (sideStr == U("sell")) side = engine::OrderSide::SELL;
                else throw std::runtime_error("Invalid order side");
                
                // Create and submit order
                auto order = std::make_shared<engine::Order>(
                    engine_->generateOrderId(),
                    1, // User ID from authentication
                    utility::conversions::to_utf8string(symbol),
                    type,
                    side,
                    price,
                    quantity
                );
                
                auto response = engine_->submitOrder(order);
                
                // Build JSON response
                json::value jsonResponse;
                jsonResponse[U("order_id")] = json::value::number(response.orderId);
                jsonResponse[U("status")] = json::value::string(
                    response.status == engine::OrderStatus::FILLED ? U("filled") :
                    response.status == engine::OrderStatus::PARTIAL ? U("partial") :
                    response.status == engine::OrderStatus::REJECTED ? U("rejected") : U("accepted")
                );
                jsonResponse[U("filled_quantity")] = json::value::number(response.filledQuantity);
                jsonResponse[U("average_price")] = json::value::number(response.averagePrice);
                jsonResponse[U("message")] = json::value::string(
                    utility::conversions::to_string_t(response.message)
                );
                
                request.reply(status_codes::OK, jsonResponse);
                
            } catch (const std::exception& e) {
                LOG_ERROR("Error submitting order: {}", e.what());
                sendErrorResponse(request, status_codes::BadRequest, 
                                std::string("Invalid order: ") + e.what());
            }
        })
        .wait();
}

void RestApi::sendErrorResponse(const http_request& request, status_code code, const std::string& message) {
    json::value response;
    response[U("error")] = json::value::string(utility::conversions::to_string_t(message));
    response[U("code")] = json::value::number(static_cast<int>(code));
    
    request.reply(code, response);
}

} // namespace api