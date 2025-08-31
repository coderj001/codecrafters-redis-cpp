#include "./include/resp_parser.h"
#include <stdexcept>
#include <string>

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

      if (input.size() < pos + len + 2 ||
          input.substr(pos + len, 2) != "\r\n") {
        throw std::runtime_error("Bulk string length mismatch");
      }
      std::string str = input.substr(pos, len);
      tokens.emplace_back(TokenType::BULKSTRING, str);
      pos += len + 2;
      break;
    }
    case '*': { // Array
      size_t end = input.find("\r\n", pos);
      int num = std::stoi(input.substr(pos + 1, end - (pos + 1)));
      pos = end + 2;
      tokens.emplace_back(TokenType::ARRAY_BEGIN, std::to_string(num));
      break;
    }
    default:
      throw std::runtime_error("Invalid RESP type: " + std::string(1, c));
    }
  }
  return tokens;
}

RESPParser::RESPParser(const std::string &string)
    : tokens(tokenizer(string)), pos(0) {}

std::shared_ptr<RESPDataType> RESPParser::parser() {
  if (pos >= tokens.size()) {
    throw std::runtime_error("Empty Input");
  }
  return parserValue();
}

size_t RESPParser::bytesConsumed() const { return pos; }

bool RESPParser::hasMore() { return pos < tokens.size(); }

Token &RESPParser::nextToken() {
  if (!hasMore()) {
    throw std::runtime_error("Unexpected end of input");
  }
  return tokens[pos++];
}

Token &RESPParser::currentToken() {
  if (!hasMore()) {
    throw std::runtime_error("Unexpected end of input");
  }
  return tokens[pos];
}

std::shared_ptr<Arrays> RESPParser::parseArray() {
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

std::shared_ptr<RESPDataType> RESPParser::parserValue() {
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
    pos--;
    return parseArray();
  default:
    throw std::runtime_error("Unsupported token in parserValue");
  }
}

std::string encodeSimpleString(const std::string &s) {
  return "+" + s + "\r\n";
}

std::string encodeBulkString(const std::string &s) {
  return "$" + std::to_string(s.size()) + "\r\n" + s + "\r\n";
}

std::string encodeNullBulkString() { return "$-1\r\n"; }
