//
// Created by 刘文景 on 2021/7/3.
//

#ifndef LJSON_LJSON_H_
#define LJSON_LJSON_H_

#include <memory>
#include <vector>
#include <string>

namespace ljson {

enum LJSON_STATE {
  LJSON_PARSE_OK = 0,
  LJSON_PARSE_EXPECT_VALUE,
  LJSON_PARSE_INVALID_VALUE,
  LJSON_PARSE_ROOT_NOT_SINGULAR,
  LJSON_PARSE_NUMBER_TOO_BIG,
  LJSON_PARSE_MISS_QUOTATION_MARK,
  LJSON_PARSE_INVALID_STRING_ESCAPE,
  LJSON_PARSE_INVALID_STRING_CHAR,
  LJSON_PARSE_INVALID_UNICODE_HEX,
  LJSON_PARSE_INVALID_UNICODE_SURROGATE,
  LJSON_PARSE_MISS_COMMA_OR_SQUARE_BRACKET,
  LJSON_PARSE_MISS_KEY,
  LJSON_PARSE_MISS_COLON,
  LJSON_PARSE_COMMA_OR_CURLY_BRACKET
};

enum LJSON_TYPE {
  LJSON_NULL = 0,
  LJSON_FALSE,
  LJSON_TRUE,
  LJSON_NUMBER,
  LJSON_STRING,
  LJSON_ARRAY,
  LJSON_OBJECT
};

typedef struct ljson_member ljson_member;

//class ljson_value {
//public:
//
//};

class ljson_value {
public:
  virtual ~ljson_value() = default;

  virtual LJSON_TYPE get_type() const = 0;

  virtual std::shared_ptr<void> get_value() const { return nullptr; }

  virtual void set_value(std::shared_ptr<void> value) = 0;

  static std::shared_ptr<ljson_value> parse(const char* json, int *ret);

};

class ljson_null : public ljson_value {
public:
  static std::shared_ptr<ljson_value> create() {
    return std::make_shared<ljson_null>();
  }

  LJSON_TYPE get_type() const override { return LJSON_NULL; }

  void set_value(std::shared_ptr<void>) override {}
};

class ljson_true : public ljson_value {
public:
  static std::shared_ptr<ljson_value> create() {
    return std::make_shared<ljson_true>();
  }

  LJSON_TYPE get_type() const override { return LJSON_TRUE; }

  void set_value(std::shared_ptr<void>) override {}
};

class ljson_false : public ljson_value {
public:
  static std::shared_ptr<ljson_value> create() {
    return std::make_shared<ljson_false>();
  }

  LJSON_TYPE get_type() const override { return LJSON_FALSE; }

  void set_value(std::shared_ptr<void>) override {}
};

class ljson_number : public ljson_value {
public:
  ljson_number() : number_(0.0) {}

  explicit ljson_number(double number) : number_(number) {}

  static std::shared_ptr<ljson_value> create(double number) {
    return std::make_shared<ljson_number>(number);
  }

  LJSON_TYPE get_type() const override { return LJSON_NUMBER; }

  std::shared_ptr<void> get_value() const override { return std::make_shared<double>(number_); }

  static double get_value_helper(const std::shared_ptr<void>& val) {
    auto ptr = std::static_pointer_cast<double>(val);
    return *ptr;
  }

  void set_value(std::shared_ptr<void> value) override {
    auto real_ptr = std::static_pointer_cast<double>(value);
    number_ = *real_ptr;
  }
private:
  double number_;

};

class ljson_string : public ljson_value {
public:
  ljson_string() = default;

  explicit ljson_string(std::string str) : str_(std::move(str)) {}

  static std::shared_ptr<ljson_value> create(std::string str = "") {
    return std::make_shared<ljson_string>(str);
  }

  LJSON_TYPE get_type() const override { return LJSON_STRING; }

  std::shared_ptr<void> get_value() const override { return std::make_shared<std::string>(str_); }

  static std::string get_value_helper(const std::shared_ptr<void>& val) {
    auto ptr = std::static_pointer_cast<std::string>(val);
    return *ptr;
  }

  void set_value(std::shared_ptr<void> value) override {
    auto real_ptr = std::static_pointer_cast<std::string>(value);
    str_ = *real_ptr;
  }
private:
  std::string str_;
};

class ljson_array : public ljson_value {
public:
  ljson_array() = default;

  explicit ljson_array(std::vector<std::shared_ptr<ljson_value>> value)
    : elements_(std::move(value)) {}

  static std::shared_ptr<ljson_value> create() {
    return std::make_shared<ljson_array>();
  }

  static std::shared_ptr<ljson_value> create(std::vector<std::shared_ptr<ljson_value>> value) {
    return std::make_shared<ljson_array>(value);
  }

  LJSON_TYPE get_type() const override { return LJSON_ARRAY; }

  std::shared_ptr<void> get_value() const override {
    return std::make_shared<std::vector<std::shared_ptr<ljson_value>>>(elements_);
  }

  static std::vector<std::shared_ptr<ljson_value>> get_value_helper(const std::shared_ptr<void>& val) {
    auto ptr = std::static_pointer_cast<std::vector<std::shared_ptr<ljson_value>>>(val);
    return *ptr;
  }

  void set_value(std::shared_ptr<void> value) override {
    auto real_ptr = std::static_pointer_cast<std::vector<std::shared_ptr<ljson_value>>>(value);
    elements_ = *real_ptr;
  }
private:
  std::vector<std::shared_ptr<ljson_value>> elements_;
};

class ljson_objects : public ljson_value {
public:
  ljson_objects() = default;

  explicit ljson_objects(std::vector<std::shared_ptr<ljson_member>> value)
    : members_(std::move(value)) {}

  static std::shared_ptr<ljson_value> create() {
    return std::make_shared<ljson_objects>();
  }

  static std::shared_ptr<ljson_value> create(std::vector<std::shared_ptr<ljson_member>> value) {
    return std::make_shared<ljson_objects>(value);
  }

  static std::vector<std::shared_ptr<ljson_member>> get_value_helper(const std::shared_ptr<void>& val) {
    auto ptr = std::static_pointer_cast<std::vector<std::shared_ptr<ljson_member>>>(val);
    return *ptr;
  }

  LJSON_TYPE get_type() const override { return LJSON_OBJECT; }

  std::shared_ptr<void> get_value() const override {
    return std::make_shared<std::vector<std::shared_ptr<ljson_member>>>(members_);
  }

  void set_value(std::shared_ptr<void> value) override {
    auto real_ptr = std::static_pointer_cast<std::vector<std::shared_ptr<ljson_member>>>(value);
    members_ = *real_ptr;
  }
private:
  std::vector<std::shared_ptr<ljson_member>> members_;
};

struct ljson_member {
  ljson_member() : key(), value(nullptr) {}
  ljson_member(std::string k, std::shared_ptr<ljson_value> val) : key(std::move(k)), value(std::move(val)) {}
  std::string key; /* member key string, key string length */
  std::shared_ptr<ljson_value> value;   /* member value */
};

} // namespace ljson

#endif //LJSON_LJSON_H_
