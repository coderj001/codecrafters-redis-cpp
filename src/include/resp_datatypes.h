#pragma once

#include <iostream>
#include <memory>
#include <string>
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
  explicit Boolean(bool initial_value) : value(initial_value) {}
  void print() const override { std::cout << (value ? "true" : "false"); }
};

class SimpleStrings : public RESPDataType {
public:
  std::string value;
  explicit SimpleStrings(const std::string &initial_value) : value(initial_value) {}
  void print() const override { std::cout << "\"" << value << "\""; }
};

class BulkStrings : public RESPDataType {
public:
  std::string value;
  explicit BulkStrings(const std::string &initial_value) : value(initial_value) {}
  void print() const override { std::cout << "\"" << value << "\""; }
};

class Integers : public RESPDataType {
public:
  long long int integer;
  explicit Integers(const long long int &initial_value) : integer(initial_value) {}
  void print() const override { std::cout << "\"" << integer << "\""; }
};

class Doubles : public RESPDataType {
public:
  double double_value;
  explicit Doubles(const double &initial_value) : double_value(initial_value) {}
  void print() const override { std::cout << "\"" << double_value << "\""; }
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
  std::string error_message;
  explicit Errors(const std::string &error_msg) : error_message(error_msg) {}
  void print() const override { std::cout << "\"" << error_message << "\""; }
};
