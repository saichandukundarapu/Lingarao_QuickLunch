#include <iostream>
#include <string>
#include <unordered_map>
#include <sstream>
#include <vector>
#include <netinet/in.h>
#include <unistd.h>

// Mock database (for demonstration)
std::unordered_map<int, std::string> menuItems = {
    {1, "Chicken Sandwich"},
    {2, "Veggie Burger"},
    {3, "Caesar Salad"},
    {4, "Grilled Cheese"},
    {5, "Pasta Carbonara"}
};

std::unordered_map<int, std::string> orders;

// Function to create an HTTP response
std::string createResponse(const std::string& status, const std::string& body) {
    std::ostringstream response;
    response << "HTTP/1.1 " << status << "\r\n";
    response << "Content-Length: " << body.size() << "\r\n";
    response << "Content-Type: application/json\r\n";
    response << "Connection: close\r\n\r\n";
    response << body;
    return response.str();
}

// Function to parse HTTP requests
std::string parseRequest(const std::string& request, const std::string& method, const std::string& endpoint) {
    std::size_t pos = request.find(method + " " + endpoint);
    if (pos != std::string::npos) {
        std::size_t startPos = request.find("\r\n\r\n", pos);
        if (startPos != std::string::npos) {
            return request.substr(startPos + 4);
        }
    }
    return "";
}

// Function to handle the menu endpoint
std::string handleGetMenu() {
    std::ostringstream body;
    body << "{ \"items\": {";
    for (const auto& item : menuItems) {
        body << "\"" << item.first << "\": \"" << item.second << "\",";
    }
    std::string bodyStr = body.str();
    bodyStr.pop_back();  // Remove last comma
    bodyStr += "}}";

    return createResponse("200 OK", bodyStr);
}

// Function to handle the order placement
std::string handlePlaceOrder(const std::string& body) {
    std::istringstream ss(body);
    std::string itemIdStr;
    ss >> itemIdStr;

    int itemId = std::stoi(itemIdStr.substr(itemIdStr.find(":") + 1));
    if (menuItems.find(itemId) == menuItems.end()) {
        return createResponse("404 Not Found", "{\"error\": \"Item Not Found\"}");
    }

    int orderId = orders.size() + 1;
    orders[orderId] = menuItems[itemId];

    std::ostringstream responseBody;
    responseBody << "{ \"order_id\": " << orderId << ", \"status\": \"Order placed successfully\", \"item\": \"" << menuItems[itemId] << "\" }";
    return createResponse("200 OK", responseBody.str());
}

// Function to handle the order status endpoint
std::string handleGetOrderStatus(int orderId) {
    if (orders.find(orderId) == orders.end()) {
        return createResponse("404 Not Found", "{\"error\": \"Order Not Found\"}");
    }

    std::ostringstream responseBody;
    responseBody << "{ \"order_id\": " << orderId << ", \"item\": \"" << orders[orderId] << "\", \"status\": \"In Progress\" }";
    return createResponse("200 OK", responseBody.str());
}

// Function to start the server
void startServer() {
    int serverFd, newSocket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};

    // Create socket file descriptor
    if ((serverFd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        std::cerr << "Socket failed" << std::endl;
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port 8080
    if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        std::cerr << "setsockopt failed" << std::endl;
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    // Bind the socket to the port
    if (bind(serverFd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Bind failed" << std::endl;
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(serverFd, 3) < 0) {
        std::cerr << "Listen failed" << std::endl;
        exit(EXIT_FAILURE);
    }

    std::cout << "Server started on port 8080" << std::endl;

    while (true) {
        if ((newSocket = accept(serverFd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
            std::cerr << "Accept failed" << std::endl;
            exit(EXIT_FAILURE);
        }

        int valread = read(newSocket, buffer, 1024);
        std::string request(buffer, valread);

        std::string response;
        if (request.find("GET /menu") == 0) {
            response = handleGetMenu();
        } else if (request.find("POST /order") == 0) {
            std::string body = parseRequest(request, "POST", "/order");
            response = handlePlaceOrder(body);
        } else if (request.find("GET /order/") == 0) {
            std::string orderIdStr = request.substr(request.find_last_of("/") + 1);
            int orderId = std::stoi(orderIdStr);
            response = handleGetOrderStatus(orderId);
        } else {
            response = createResponse("404 Not Found", "{\"error\": \"Endpoint Not Found\"}");
        }

        send(newSocket, response.c_str(), response.size(), 0);
        close(newSocket);
    }
}

int main() {
    startServer();
    return 0;
}
