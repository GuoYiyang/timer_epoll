#ifndef ERROR_H_
#define ERROR_H_
#include <cassert>
#include <iostream>
#include <map>
#include <string>

class Error {
 public:
  Error(int value, const std::string& str) {
    value_ = value;
    message_ = str;
#ifdef _DEBUG
    ErrorMap::iterator found = GetErrorMap().find(value);
    if (found != GetErrorMap().end()) {
      assert(found->second == message_);
    }
#endif
    GetErrorMap()[value_] = message_;
  }

  // auto-cast Error to integer error code
  operator int() { return value_; }

 private:
  int value_;
  std::string message_;

  typedef std::map<int, std::string> ErrorMap;
  static ErrorMap& GetErrorMap() {
    static ErrorMap errMap;
    return errMap;
  }

 public:
  static std::string GetErrorString(int value) {
    ErrorMap::iterator found = GetErrorMap().find(value);
    if (found == GetErrorMap().end()) {
      assert(false);
      return "";
    } else {
      return found->second;
    }
  }
};
#endif