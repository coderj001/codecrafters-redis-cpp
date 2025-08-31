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
  explicit Boolean(bool v) : value(v) {}
  void print() const override { std::cout << (value ? "true" : "false"); }
};

class SimpleStrings : public RESPDataType {
public:
  std::string value;
  explicit SimpleStrings(const std::string &v) : value(v) {}
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
  explicit Integers(const long long int &v) : integer(v) {}
  void print() const override { std::cout << "\"" << integer << "\""; }
};

class Doubles : public RESPDataType {
public:
  double d;
  explicit Doubles(const double &v) : d(v) {}
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
  explicit Errors(const std::string &err) : err(err) {}
  void print() const override { std::cout << "\"" << err << "\""; }
};
