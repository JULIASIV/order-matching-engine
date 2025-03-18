#ifndef ORDERBOOK_H
#define ORDERBOOK_H

#include <unordered_map>
#include <vector>
#include <string>
#include <mutex>
#include "Order.h"

class OrderBook {
private:
    std::unordered_map<double, std::vector<Order>> buyOrders;
    std::unordered_map<double, std::vector<Order>> sellOrders;
    std::atomic<int> orderCounter{0};
    std::mutex orderBookMutex;

public:
    void addOrder(char type, double price, int quantity);
    void matchOrders();
    std::string getOrderBook();
};

#endif // ORDERBOOK_H
