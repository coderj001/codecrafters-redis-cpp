#pragma once

#include "./resp_datatypes.h"

#include <string>
#include <vector>

/**
 * TokenType: Types RESP token
 */
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
  size_t length;

  Token(TokenType token_type, const std::string &token_value, size_t token_length)
      : type(token_type), value(token_value), length(token_length) {}
};

/**
 * Tokenizes a RESP input string into tokens.
 */
std::vector<Token> tokenizer(const std::string &input);

class RESPParser {
public:
  explicit RESPParser(const std::string &string);

  // Non-copyable, moveable
  RESPParser(const RESPParser &) = delete;
  RESPParser &operator=(const RESPParser &) = delete;
  RESPParser(RESPParser &&) = default;
  RESPParser &operator=(RESPParser &&) = default;

  std::shared_ptr<RESPDataType> parse();
  size_t bytesConsumed() const;

private:
  std::vector<Token> tokens;
  size_t current_position;

  bool hasMore();
  Token &nextToken();
  Token &currentToken();
  std::shared_ptr<Arrays> parseArray();
  std::shared_ptr<RESPDataType> parse_value();
  size_t getConsumedBytes() const;
};

/**
 * RESP encoding helpers
 */
std::string encodeSimpleString(const std::string &input_string);
std::string encodeBulkString(const std::string &input_string);
std::string encodeNullBulkString();
std::string encodeErrorString(const std::string &error_message);
