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
  LJSON_PARSE_ROOT_NOT_SINGULAR
};

enum LJSON_TYPE {
  LJSON_NULL,
  LJSON_FALSE,
  LJSON_TRUE,
  LJSON_NUMBER,
  LJSON_STRING,
  LJSON_ARRAY,
  LJSON_OBJECT
};
class ljson_value : public std::enable_shared_from_this<ljson_value> {
public:


  static std::shared_ptr<ljson_value> create();

  int parse(const char *json);

  std::shared_ptr<ljson_value> getptr() {
    return shared_from_this();
  }

  LJSON_TYPE get_type() const { return type_; }

  void set_type(LJSON_TYPE type) { type_ = type; }

protected:

  ljson_value() : type_(LJSON_NULL) {}

private:

  LJSON_TYPE type_;
};

} // namespace ljson

#endif //LJSON_LJSON_H_
