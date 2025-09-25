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

  Token(TokenType t, const std::string &v, size_t l)
      : type(t), value(v), length(l) {}
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

  std::shared_ptr<RESPDataType> parser();
  size_t bytesConsumed() const;

private:
  std::vector<Token> tokens;
  size_t pos;

  bool hasMore();
  Token &nextToken();
  Token &currentToken();
  std::shared_ptr<Arrays> parseArray();
  std::shared_ptr<RESPDataType> parserValue();
  size_t getConsumedBytes() const;
};

/**
 * RESP encoding helpers
 */
std::string encodeSimpleString(const std::string &s);
std::string encodeBulkString(const std::string &s);
std::string encodeNullBulkString();
std::string encodeErrorString(const std::string &err);
