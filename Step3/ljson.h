//
// Created by 刘文景 on 2021/7/3.
//

#ifndef LJSON_LJSON_H_
#define LJSON_LJSON_H_

#include <memory>

namespace ljson {

enum LJSON_STATE {
  LJSON_PARSE_OK = 0,
  LJSON_PARSE_EXPECT_VALUE,
  LJSON_PARSE_INVALID_VALUE,
  LJSON_PARSE_ROOT_NOT_SINGULAR,
  LJSON_PARSE_NUMBER_TOO_BIG,
  LJSON_PARSE_MISS_QUOTATION_MARK,
  LJSON_PARSE_INVALID_STRING_ESCAPE,
  LJSON_PARSE_INVALID_STRING_CHAR
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
class ljson_value : public std::enable_shared_from_this<ljson_value> {
public:
  ~ljson_value();

  static std::shared_ptr<ljson_value> create();

  int parse(const char *json);

  std::shared_ptr<ljson_value> getptr() {
    return shared_from_this();
  }

  LJSON_TYPE get_type() const { return type_; }

  double get_number() const { return number_; }

  const char* get_string() const;

  size_t get_string_length() const;

  void set_type(LJSON_TYPE type) { type_ = type; }

  void set_number(double number) { set_type(LJSON_NUMBER), number_ = number; }

  void set_string(const char* s, size_t len);

protected:

  ljson_value() : type_(LJSON_NULL) {}

private:

  void destroy();

  void assert_is_string() const {
    assert(type_ == LJSON_STRING);
  }

  LJSON_TYPE type_;

  struct ljson_string {
    ljson_string() : str(nullptr), len(0) {}
    char *str;
    size_t len;
  };

  union {
    ljson_string str_;
    double number_{0.0};
  };
};

} // namespace ljson

#endif //LJSON_LJSON_H_
