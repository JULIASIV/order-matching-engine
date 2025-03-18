#include <fstream>

void saveOrderBook(OrderBook& ob) {
    std::ofstream outfile("order_book.txt");
    outfile << ob.getOrderBook();
    outfile.close();
}

void loadOrderBook(OrderBook& ob) {
    std::ifstream infile("order_book.txt");
    std::string line;
    while (std::getline(infile, line)) {
        // Parse and load orders into the order book
    }
}

