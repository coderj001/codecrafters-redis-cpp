#pragma once

#include "./resp_datatypes.h"

#include <string>
#include <vector>

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

std::vector<Token> tokenizer(const std::string &input);

class RESPParser {
public:
  explicit RESPParser(const std::string &string);
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

std::string encodeSimpleString(const std::string &s);
std::string encodeBulkString(const std::string &s);
std::string encodeNullBulkString();
