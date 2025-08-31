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

class Null : public RESPDataType {
public:
  void print() const override { std::cout << "null"; }
};

class Boolean : public RESPDataType {
public:
  bool value;
  explicit Boolean(bool v) : value(v) {};
  void print() const override { std::cout << (value ? "true" : "false"); }
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

class Doubles : public RESPDataType {
public:
  double d;
  explicit Doubles(const double &v) : d(v) {};
  void print() const override { std::cout << "\"" << d << "\""; }
};

class Arrays : public RESPDataType {
public:
  std::vector<std::shared_ptr<RESPDataType>> values;
  void print() const {
    std::cout << "[";
    for (size_t i = 0; i < values.size(); i++) {
      values[i]->print();
      if (i < values.size() - 1) {
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

enum TokenType {
  STRING,
  INTEGER,
  DOUBLE,
  BULKSTRING,
  ERROR,
  NULLs,
  ARRAY_BEGIN,
  ARRAY_END
};

struct Token {
  TokenType type;
  std::string value;

  Token(TokenType t, const std::string &v) : type(t), value(v) {}
};

std::vector<Token> tokenizer(const std::string &input) {
  std::vector<Token> tokens;
  size_t pos = 0;

  while (pos < input.size()) {
    char c = input[pos];
    switch (c) {
    case '+': { // Simple String
      size_t end = input.find("\r\n", pos);
      std::string str = input.substr(pos + 1, end - (pos + 1));
      tokens.emplace_back(TokenType::STRING, str);
      pos = end + 2;
      break;
    }
    case '-': { // Error
      size_t end = input.find("\r\n", pos);
      std::string str = input.substr(pos + 1, end - (pos + 1));
      tokens.emplace_back(TokenType::ERROR, str);
      pos = end + 2;
      break;
    }
    case ':': { // Integer
      size_t end = input.find("\r\n", pos);
      std::string num = input.substr(pos + 1, end - (pos + 1));
      tokens.emplace_back(TokenType::INTEGER, num);
      pos = end + 2;
      break;
    }
    case ',': { // Double
      size_t end = input.find("\r\n", pos);
      std::string num = input.substr(pos + 1, end - (pos + 1));
      tokens.emplace_back(TokenType::DOUBLE, num);
      pos = end + 2;
      break;
    }
    case '_': { // Null
      tokens.emplace_back(TokenType::NULLs, "");
      pos += 3; // "_\r\n"
      break;
    }
    case '#': { // Boolean
      char val = input[pos + 1];
      tokens.emplace_back(TokenType::STRING, val == 't' ? "true" : "false");
      pos += 3; // "#t\r\n" or "#f\r\n"
      break;
    }
    case '$': { // Bulk String
      size_t end = input.find("\r\n", pos);
      int len = std::stoi(input.substr(pos + 1, end - (pos + 1)));
      pos = end + 2;

      if (len == -1) {
        tokens.emplace_back(TokenType::BULKSTRING, ""); // Null Bulk String
      } else {
        std::string str = input.substr(pos, len);
        tokens.emplace_back(TokenType::BULKSTRING, str);
        pos += len + 2; // skip string + \r\n
      }
      break;
    }
    case '*': { // Array
      size_t end = input.find("\r\n", pos);
      int num = std::stoi(input.substr(pos + 1, end - (pos + 1)));
      pos = end + 2;
      tokens.emplace_back(TokenType::ARRAY_BEGIN, std::to_string(num));
      break; // no ARRAY_END here
    }
    default:
      throw std::runtime_error(std::string("Invalid RESP type: ") + c);
    }
  }
  return tokens;
}

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

  Token &currentToken() {
    if (!hasMore()) {
      throw std::runtime_error("Unexpected end of input");
    }
    return tokens[pos];
  }

  std::shared_ptr<Arrays> parseArray() {
    auto array = std::make_shared<Arrays>();

    Token &begin = nextToken();
    if (begin.type != TokenType::ARRAY_BEGIN) {
      throw std::runtime_error("Expected ARRAY_BEGIN");
    }

    int num = std::stoi(begin.value);
    for (int i = 0; i < num; i++) {
      array->values.emplace_back(parserValue());
    }
    return array;
  }

  std::shared_ptr<RESPDataType> parserValue() {
    Token &tok = nextToken();
    switch (tok.type) {
    case STRING:
      return std::make_shared<SimpleStrings>(tok.value);
    case BULKSTRING:
      return std::make_shared<BulkStrings>(tok.value);
    case INTEGER:
      return std::make_shared<Integers>(std::stoll(tok.value));
    case DOUBLE:
      return std::make_shared<Doubles>(std::stod(tok.value));
    case ERROR:
      return std::make_shared<Errors>(tok.value);
    case NULLs:
      return std::make_shared<Null>();
    case ARRAY_BEGIN:
      pos--; // step back so parseArray() sees it
      return parseArray();
    default:
      throw std::runtime_error("Unsupported token in parserValue");
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
    } else if (received_data.find("ECHO") == 0) {
      std::string arg = received_data.substr(5);
      if (!arg.empty() && arg.back() == '\n')
        arg.pop_back();
      if (!arg.empty() && arg.back() == '\r')
        arg.pop_back();

      std::string resp =
          "$" + std::to_string(arg.size()) + "\r\n" + arg + "\r\n";

      write(client_fd, resp.c_str(), resp.size());
    } else {
      RESPParser parser(received_data);

      auto root = parser.parser();
      auto arr = std::dynamic_pointer_cast<Arrays>(root);

      std::string cmd =
          std::dynamic_pointer_cast<BulkStrings>(arr->values[0])->value;
      if (cmd == "ECHO") {
        std::string resp =
            std::dynamic_pointer_cast<BulkStrings>(arr->values[1])->value;

        write(client_fd, resp.c_str(), resp.length());
      }
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

  // Main loop that accept connection
  while (true) {
    struct sockaddr_in client_addr;
    int client_addr_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (struct sockaddr *)&client_addr,
                           (socklen_t *)&client_addr_len);

    if (client_fd < 0) {
      std::cerr << "Failed to accept client connection\n";
    }

    std::thread client_thread(handle_client,
                              client_fd); // create a new thread
    client_thread.detach();               // let's it run independently
  }

  close(server_fd);

  return 0;
}
