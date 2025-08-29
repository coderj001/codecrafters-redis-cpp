#include <arpa/inet.h>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <netdb.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <strstream>
#include <sys/socket.h>
#include <sys/types.h>
#include <system_error>
#include <thread>
#include <unistd.h>
#include <vector>

class RESPDataType {
public:
  virtual ~RESPDataType() = default;
  virtual void print() const = 0;
};

class SimpleStrings : public RESPDataType {
public:
  std::string value;
  explicit SimpleStrings(const std::string &v) : value(v) {};
  void print() const override { std::cout << "\"" << value << "\""; }
};

class BulkStrings : public RESPDataType {
public:
  std::string value;
  explicit BulkStrings(const std::string &v) : value(v) {}
  void print() const override { std::cout << "\"" << value << "\""; }
};

class Integers : public RESPDataType {
public:
  long long int integer;
  explicit Integers(const long long int &v) : integer(v) {};
  void print() const override { std::cout << "\"" << integer << "\""; }
};

class Arrays : public RESPDataType {
public:
  std::vector<std::shared_ptr<RESPDataType>> arrays;
  void print() const {
    std::cout << "[";
    for (size_t i = 0; i < arrays.size(); i++) {
      arrays[i]->print();
      if (i < arrays.size() - 1) {
        std::cout << ", ";
      }
    }
    std::cout << "]";
  }
};

class Errors : public RESPDataType {
public:
  std::string err;
  explicit Errors(const std::string &err) : err(err) {};
  void print() const override { std::cout << "\"" << err << "\""; }
};

enum TokenType { STRING, ARRAY_BEGIN, ARRAY_END };

struct Token {
  TokenType type;
  std::string value;

  Token(TokenType t, const std::string &v) : type(t), value(v) {}
};

std::vector<Token> tokenizer(const std::string &input) {
  std::vector<Token> tokens;
  size_t pos = 0;

  while (pos < input.length()) {
    char c = input[pos];
    switch (c) {
    case '+':
      size_t p = input.find("\r\n");
      std::string str = input.substr(pos + 1, p - (pos + 1));
      tokens.emplace_back(TokenType::STRING, str);
      pos = pos + str.length() + 2;
      break;
    case '*':
      p++;
      size_t num_of_items = input[pos];
      p++;
      tokens.emplace_back(TokenType::ARRAY_BEGIN, "[");
      for (size_t i = 0; i < num_of_items; i++) {
        std::string str = input.substr(pos, input.find("\r\n"));
        tokens.emplace_back(TokenType::STRING, str);
        pos = pos + str.length() + 2;
      }
      tokens.emplace_back(TokenType::ARRAY_END, "]");
      break;
    }
  }

  return tokens;
};

class RESPParser {
public:
  explicit RESPParser(const std::string &string)
      : tokens(tokenizer(string)), pos(0) {};

  std::shared_ptr<RESPDataType> parser() {
    if (pos >= tokens.size()) {
      throw std::runtime_error("Empty Input");
    }
    return parserValue();
  }

private:
  std::vector<Token> tokens;
  size_t pos;

  bool hasMore() { return pos < tokens.size(); }

  Token &nextToken() {
    if (!hasMore()) {
      throw std::runtime_error("Unexpected end of input");
    }
    return tokens[pos++];
  }

  std::shared_ptr<RESPDataType> parserValue() {
    Token &tok = nextToken();
    switch (tok.type) {
    case STRING:
      return std::make_shared<SimpleStrings>(tok.value);
    case ARRAY_BEGIN:

    default:

      throw std::runtime_error("Unsupported token");
    }
  }
};

void handle_client(int client_fd) {
  std::cout << "Client connected on thread " << std::this_thread::get_id()
            << std::endl;
  ;
  char buffer[1024] = {0}; // hold data from client

  while (true) {
    ssize_t bytes_received = read(client_fd, buffer, sizeof(buffer));

    if (bytes_received < 0) {
      std::cerr << "Failed to read\n";
    }

    std::string received_data(buffer, bytes_received);

    if (received_data.find("PING") != std::string::npos) {
      const char *resp = "+PONG\r\n";
      write(client_fd, resp, strlen(resp));
    } else if (received_data.find("END") != std::string::npos) {
      break;
    }
  }

  close(client_fd);
}

int main(int argc, char **argv) {
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    std::cerr << "Failed to create server socket\n";
    return 1;
  }

  // Since the tester restarts your program quite often, setting SO_REUSEADDR
  // ensures that we don't run into 'Address already in use' errors
  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) <
      0) {
    std::cerr << "setsockopt failed\n";
    return 1;
  }

  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(6379);

  if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) !=
      0) {
    std::cerr << "Failed to bind to port 6379\n";
    return 1;
  }

  int connection_backlog = 5;
  if (listen(server_fd, connection_backlog) != 0) {
    std::cerr << "Listen failed\n";
    return 1;
  }

  // struct sockaddr_in client_addr;
  // int client_addr_len = sizeof(client_addr);
  // std::cout << "Waiting for a client to connect...\n";
  // std::cout << "Logs from your program will appear here!\n";
  //
  // int client_fd = accept(server_fd, (struct sockaddr *)&client_addr,
  //                        (socklen_t *)&client_addr_len);
  // if (client_fd < 0) {
  //   std::cerr << "Failed to accept client connection\n";
  // }
  //
  // handle_client(client_fd);

  // Main loop that accept connection
  while (true) {
    struct sockaddr_in client_addr;
    int client_addr_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (struct sockaddr *)&client_addr,
                           (socklen_t *)&client_addr_len);

    if (client_fd < 0) {
      std::cerr << "Failed to accept client connection\n";
    }

    std::thread client_thread(handle_client, client_fd); // create a new thread
    client_thread.detach(); // let's it run independently
  }

  close(server_fd);

  return 0;
}
