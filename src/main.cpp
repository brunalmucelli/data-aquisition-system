#include <boost/asio.hpp>
#include <iostream>
#include <fstream>
#include <memory>
#include <string>

using boost::asio::ip::tcp;

class Session : public std::enable_shared_from_this<Session> {
public:
    explicit Session(tcp::socket socket) : socket_(std::move(socket)) {}

    void start() {
        read();
    }

private:
    tcp::socket socket_;
    std::string data_;

    void read() {
        auto self(shared_from_this());
        boost::asio::async_read_until(socket_, boost::asio::dynamic_buffer(data_), '\n',
            [this, self](boost::system::error_code ec, std::size_t length) {
                if (!ec) {
                    handle_data(data_.substr(0, length));
                    data_.erase(0, length);
                    read(); // Continue reading
                }
            });
    }

    void handle_data(const std::string& data) {
        std::cout << "Received: " << data << std::endl;

        // Write data to file
        std::ofstream file("sensor_data.bin", std::ios::app | std::ios::binary);
        if (file.is_open()) {
            file.write(data.c_str(), data.size());
            file.close();
        }
    }
};

class Server {
public:
    Server(boost::asio::io_context& io_context, short port)
        : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)) {
        accept();
    }

private:
    tcp::acceptor acceptor_;

    void accept() {
        acceptor_.async_accept(
            [this](boost::system::error_code ec, tcp::socket socket) {
                if (!ec) {
                    std::make_shared<Session>(std::move(socket))->start();
                }
                accept(); // Accept next connection
            });
    }
};

int main() {
    try {
        boost::asio::io_context io_context;
        Server server(io_context, 12345);
        std::cout << "Server is running on port 12345..." << std::endl;
        io_context.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return 0;
}
