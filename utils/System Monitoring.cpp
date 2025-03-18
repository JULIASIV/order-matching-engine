#include <spdlog/spdlog.h>

void logOrderProcessed(int orderId, int quantity) {
    spdlog::info("Processed Order {}: {} units", orderId, quantity);
}

