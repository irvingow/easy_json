//
// Created by 刘文景 on 2021/7/3.
//

#ifndef LJSON_LJSON_H_
#define LJSON_LJSON_H_

#include <memory>
#include <vector>

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

  size_t get_array_size() const;

  std::shared_ptr<ljson_value> get_array_element(size_t index) const;

  size_t get_objects_size() const;

  const char* get_object_key(size_t index) const;

  size_t get_object_key_length(size_t index) const;

  std::shared_ptr<ljson_value> get_object_value(size_t index);

  void set_type(LJSON_TYPE type) { type_ = type; }

  void set_number(double number) {  type_ = LJSON_NUMBER, number_ = number; }

  void set_string(const char* s, size_t len);

  void set_array(std::vector<std::shared_ptr<ljson_value>> array);

//  void array_push(std::shared_ptr<ljson_value> value);

  void set_objects(std::vector<std::shared_ptr<ljson_member>> members);

//  void object_push(std::shared_ptr<ljson_member> member);

protected:

  ljson_value() : type_(LJSON_NULL) {}

private:

  void destroy();

  void assert_type(LJSON_TYPE type) const {
    assert(type_ == type);
  }

  LJSON_TYPE type_;

  struct ljson_string {
    ljson_string() : str(nullptr), len(0) {}
    char *str;
    size_t len;
  };

  union {
    ljson_string str_;            /* string: null-terminated string, string length */
    double number_{0.0};          /* number */
  };
  /// move vector outside union
  /// see reference: https://en.wikipedia.org/wiki/C%2B%2B11#Unrestricted_unions
  std::vector<std::shared_ptr<ljson_value>> elements_;
  std::vector<std::shared_ptr<ljson_member>> members_;
};

struct ljson_member {
  ljson_member() : k(nullptr), len(0), value(ljson_value::create()) {}
  ~ljson_member() { free(k); }
  char *k; size_t len; /* member key string, key string length */
  std::shared_ptr<ljson_value> value;   /* member value */
};

} // namespace ljson

#endif //LJSON_LJSON_H_
