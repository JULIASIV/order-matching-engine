// include/feeds/WebSocketFeed.hpp
#pragma once

#include "MarketDataFeed.hpp"
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/client.hpp>
#include <websocketpp/common/thread.hpp>

namespace feeds {

class WebSocketFeed : public MarketDataFeed {
public:
    WebSocketFeed(std::shared_ptr<networking::ZmqInterface> zmqInterface,
                  const std::string& uri, const std::string& feedConfig);
    ~WebSocketFeed();
    
    void start() override;
    void stop() override;
    
    void onQuote(const std::string& symbol, double bid, double ask, 
                int64_t bidSize, int64_t askSize) override;
    void onTrade(const std::string& symbol, double price, 
                int64_t quantity, const std::string& timestamp) override;
    void onDepthUpdate(const std::string& symbol, 
                      const std::vector<std::pair<double, int64_t>>& bids,
                      const std::vector<std::pair<double, int64_t>>& asks) override;
    
private:
    using WebSocketClient = websocketpp::client<websocketpp::config::asio>;
    
    WebSocketClient client_;
    websocketpp::connection_hdl connection_;
    std::string uri_;
    std::string feedConfig_;
    websocketpp::lib::shared_ptr<websocketpp::lib::thread> clientThread_;
    
    void runFeed() override;
    void onMessage(websocketpp::connection_hdl hdl, WebSocketClient::message_ptr msg);
    void onOpen(websocketpp::connection_hdl hdl);
    void onClose(websocketpp::connection_hdl hdl);
    
    void parseMessage(const std::string& message);
    void sendSubscriptionMessage();
};

} // namespace feeds