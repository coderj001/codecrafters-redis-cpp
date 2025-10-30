#include "./include/resp_parser.h"
#include <stdexcept>
#include <string>

std::vector<Token> tokenizer(const std::string &input) {
  std::vector<Token> tokens;
  size_t current_position = 0;

  while (current_position < input.size()) {
    size_t token_start_position = current_position;
    char current_char = input[current_position];
    switch (current_char) {
    case '+': { // Simple String
      size_t line_end_position = input.find("\r\n", current_position);
      std::string string_content = input.substr(current_position + 1, line_end_position - (current_position + 1));
      tokens.emplace_back(TokenType::STRING, string_content, line_end_position - token_start_position + 2);
      current_position = line_end_position + 2;
      break;
    }
    case '-': { // Error
      size_t line_end_position = input.find("\r\n", current_position);
      std::string string_content = input.substr(current_position + 1, line_end_position - (current_position + 1));
      tokens.emplace_back(TokenType::ERROR, string_content, line_end_position - token_start_position + 2);
      current_position = line_end_position + 2;
      break;
    }
    case ':': { // Integer
      size_t line_end_position = input.find("\r\n", current_position);
      std::string numeric_string = input.substr(current_position + 1, line_end_position - (current_position + 1));
      tokens.emplace_back(TokenType::INTEGER, numeric_string, line_end_position - token_start_position + 2);
      current_position = line_end_position + 2;
      break;
    }
    case ',': { // Double
      size_t line_end_position = input.find("\r\n", current_position);
      std::string numeric_string = input.substr(current_position + 1, line_end_position - (current_position + 1));
      tokens.emplace_back(TokenType::DOUBLE, numeric_string, line_end_position - token_start_position + 2);
      current_position = line_end_position + 2;
      break;
    }
    case '_': { // Null
      tokens.emplace_back(TokenType::NULLs, "", 3);
      current_position += 3; // "_\r\n"
      break;
    }
    case '#': { // Boolean
      char boolean_value = input[current_position + 1];
      tokens.emplace_back(TokenType::STRING, boolean_value == 't' ? "true" : "false", 3);
      current_position += 3; // "#t\r\n" or "#f\r\n"
      break;
    }
    case '$': { // Bulk String
      size_t line_end_position = input.find("\r\n", current_position);
      int bulk_string_length = std::stoi(input.substr(current_position + 1, line_end_position - (current_position + 1)));
      current_position = line_end_position + 2;
      if (bulk_string_length == -1) {
        tokens.emplace_back(TokenType::NULLs, "", line_end_position - token_start_position + 2);
      } else {
        if (input.size() < current_position + bulk_string_length + 2 ||
            input.substr(current_position + bulk_string_length, 2) != "\r\n") {
          throw std::runtime_error("Bulk string length mismatch");
        }
        std::string string_content = input.substr(current_position, bulk_string_length);
        tokens.emplace_back(TokenType::BULKSTRING, string_content,
                            line_end_position - token_start_position + 2 + bulk_string_length + 2);
        current_position += bulk_string_length + 2;
      }
      break;
    }
    case '*': { // Array
      size_t line_end_position = input.find("\r\n", current_position);
      int array_element_count = std::stoi(input.substr(current_position + 1, line_end_position - (current_position + 1)));
      current_position = line_end_position + 2;
      tokens.emplace_back(TokenType::ARRAY_BEGIN, std::to_string(array_element_count),
                          line_end_position - token_start_position + 2);
      break;
    }
    default:
      throw std::runtime_error("Invalid RESP type: " + std::string(1, current_char));
    }
  }
  return tokens;
}

RESPParser::RESPParser(const std::string &string)
    : tokens(tokenizer(string)), current_position(0) {}

std::shared_ptr<RESPDataType> RESPParser::parse() {
  if (current_position >= tokens.size()) {
    throw std::runtime_error("Empty Input");
  }
  return parse_value();
}

size_t RESPParser::bytesConsumed() const { return getConsumedBytes(); }

size_t RESPParser::getConsumedBytes() const {
  size_t total_bytes = 0;
  for (size_t i = 0; i < current_position; ++i) {
    total_bytes += tokens[i].length;
  }
  return total_bytes;
}

bool RESPParser::hasMore() { return current_position < tokens.size(); }

Token &RESPParser::nextToken() {
  if (!hasMore()) {
    throw std::runtime_error("Unexpected end of input");
  }
  return tokens[current_position++];
}

Token &RESPParser::currentToken() {
  if (!hasMore()) {
    throw std::runtime_error("Unexpected end of input");
  }
  return tokens[current_position];
}

std::shared_ptr<Arrays> RESPParser::parseArray() {
  auto array = std::make_shared<Arrays>();

  Token &begin = nextToken();
  if (begin.type != TokenType::ARRAY_BEGIN) {
    throw std::runtime_error("Expected ARRAY_BEGIN");
  }

  int array_element_count = std::stoi(begin.value);
  for (int i = 0; i < array_element_count; i++) {
    array->values.emplace_back(parse_value());
  }
  return array;
}

std::shared_ptr<RESPDataType> RESPParser::parse_value() {
  Token &current_token = nextToken();
  switch (current_token.type) {
  case STRING:
    return std::make_shared<SimpleStrings>(current_token.value);
  case BULKSTRING:
    return std::make_shared<BulkStrings>(current_token.value);
  case INTEGER:
    return std::make_shared<Integers>(std::stoll(current_token.value));
  case DOUBLE:
    return std::make_shared<Doubles>(std::stod(current_token.value));
  case ERROR:
    return std::make_shared<Errors>(current_token.value);
  case NULLs:
    return std::make_shared<Null>();
  case ARRAY_BEGIN:
    current_position--;
    return parseArray();
  default:
    throw std::runtime_error("Unsupported token in parse_value");
  }
}

std::string encodeSimpleString(const std::string &input_string) {
  return "+" + input_string + "\r\n";
}

std::string encodeBulkString(const std::string &input_string) {
  return "$" + std::to_string(input_string.size()) + "\r\n" + input_string + "\r\n";
}

std::string encodeNullBulkString() { return "$-1\r\n"; }

std::string encodeErrorString(const std::string &error_message) {
  return "-" + error_message + "\r\n";
}
