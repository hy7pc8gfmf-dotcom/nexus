#ifndef NLOHMANN_JSON_MINIMAL_HPP
#define NLOHMANN_JSON_MINIMAL_HPP

/**
 * @file json.hpp — Minimal JSON library for Nexus
 *
 * 这是 nlohmann/json 的最小化替代品。
 * 只实现了 Nexus 项目使用的 API 子集。
 *
 * 当 vcpkg 可用时，将此文件替换为官方的单头文件版本：
 *   https://github.com/nlohmann/json/releases
 *
 * API 兼容性: 实现以下核心接口:
 *   json::object(), json::array(), parse()
 *   operator[], at(), value(), dump(), get<T>()
 *   is_object(), is_array(), is_string(), is_number()
 *   size(), empty(), begin(), end()
 *   >> operator (istream)
 *
 * 许可证: MIT (与原始 nlohmann/json 一致)
 */

#include <cstdint>
#include <cstdlib>
#include <exception>
#include <initializer_list>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

namespace nlohmann {

// ═══════════════════════════════════════════════════════════════════
// 异常类型
// ═══════════════════════════════════════════════════════════════════

struct json_exception : public std::exception {
  std::string msg_;
  explicit json_exception(std::string msg) : msg_(std::move(msg)) {}
  const char* what() const noexcept override { return msg_.c_str(); }
};

struct parse_error : public json_exception {
  explicit parse_error(std::string msg) : json_exception(std::move(msg)) {}
};

struct type_error : public json_exception {
  explicit type_error(std::string msg) : json_exception(std::move(msg)) {}
};

// ═══════════════════════════════════════════════════════════════════
// JSON 值类型
// ═══════════════════════════════════════════════════════════════════

enum class json_type : uint8_t {
  null_, bool_, int_, uint_, double_, string_, array_, object_
};

class json {
public:
  // ── 构造 ──
  json() noexcept : type_(json_type::null_) {}
  json(std::nullptr_t) noexcept : type_(json_type::null_) {}
  json(bool v) noexcept : type_(json_type::bool_), bool_val_(v) {}
  json(int v) noexcept : type_(json_type::int_), int_val_(v) {}
  json(unsigned v) noexcept : type_(json_type::uint_), uint_val_(v) {}
  json(long v) noexcept : type_(json_type::int_), int_val_(static_cast<int64_t>(v)) {}
  json(unsigned long v) noexcept : type_(json_type::uint_), uint_val_(static_cast<uint64_t>(v)) {}
  json(long long v) noexcept : type_(json_type::int_), int_val_(static_cast<int64_t>(v)) {}
  json(unsigned long long v) noexcept : type_(json_type::uint_), uint_val_(static_cast<uint64_t>(v)) {}
  json(double v) noexcept : type_(json_type::double_), double_val_(v) {}
  json(const char* v) : type_(json_type::string_), str_val_(v) {}
  json(std::string v) noexcept : type_(json_type::string_), str_val_(std::move(v)) {}

  // initializer_list 构造（支持嵌套的 {key, value} 对 或 数组元素）
  json(std::initializer_list<json> list) {
    bool is_object = true;
    bool is_array = false;
    size_t count = 0;

    for (auto it = list.begin(); it != list.end() && count < 2; ++it, ++count) {
      // 空判断
    }

    // 所有元素都是 2 元素数组 → 对象
    for (const auto& item : list) {
      if (!item.is_array() || item.size() != 2) {
        is_object = false;
        break;
      }
    }

    if (list.size() == 0) {
      type_ = json_type::null_;
    } else if (is_object) {
      type_ = json_type::object_;
      obj_val_ = map_type();
      for (const auto& item : list) {
        if (item.is_array() && item.size() == 2) {
          auto key = item[0].get<std::string>();
          (*obj_val_)[std::move(key)] = item[1];
        }
      }
    } else {
      type_ = json_type::array_;
      arr_val_ = std::vector<json>(list);
    }
  }

  // 复制/移动
  json(const json&) = default;
  json(json&&) noexcept = default;
  json& operator=(const json&) = default;
  json& operator=(json&&) noexcept = default;
  ~json() = default;

  // ── 工厂 ──
  static json object() {
    json j;
    j.type_ = json_type::object_;
    j.obj_val_ = map_type();
    return j;
  }

  static json array() {
    json j;
    j.type_ = json_type::array_;
    j.arr_val_ = std::vector<json>();
    return j;
  }

  static json parse(const std::string& s) {
    size_t pos = 0;
    return parse_value(s, pos);
  }

  // ── 类型查询 ──
  bool is_null()   const noexcept { return type_ == json_type::null_; }
  bool is_boolean() const noexcept { return type_ == json_type::bool_; }
  bool is_number_integer() const noexcept { return type_ == json_type::int_; }
  bool is_number_unsigned() const noexcept { return type_ == json_type::uint_; }
  bool is_number_float() const noexcept { return type_ == json_type::double_; }
  bool is_number() const noexcept {
    return type_ == json_type::int_ || type_ == json_type::uint_ ||
           type_ == json_type::double_;
  }
  bool is_string() const noexcept { return type_ == json_type::string_; }
  bool is_array()  const noexcept { return type_ == json_type::array_; }
  bool is_object() const noexcept { return type_ == json_type::object_; }

  // ── 访问 ──
  json& operator[](const std::string& key) {
    if (type_ != json_type::object_) {
      type_ = json_type::object_;
      obj_val_ = map_type();
    }
    return (*obj_val_)[key];
  }

  json& operator[](size_t idx) {
    if (type_ != json_type::array_) {
      type_ = json_type::array_;
      arr_val_ = std::vector<json>();
    }
    if (idx >= arr_val_->size()) {
      arr_val_->resize(idx + 1);
    }
    return (*arr_val_)[idx];
  }

  const json& operator[](const std::string& key) const {
    return at(key);
  }

  const json& operator[](size_t idx) const {
    return at(idx);
  }

  const json& at(const std::string& key) const {
    if (type_ != json_type::object_) throw type_error("not an object");
    auto it = obj_val_->find(key);
    if (it == obj_val_->end()) throw type_error("key not found: " + key);
    return it->second;
  }

  json& at(const std::string& key) {
    return const_cast<json&>(const_cast<const json*>(this)->at(key));
  }

  const json& at(size_t idx) const {
    if (type_ != json_type::array_) throw type_error("not an array");
    if (idx >= arr_val_->size()) throw type_error("index out of range");
    return (*arr_val_)[idx];
  }

  json& at(size_t idx) {
    return const_cast<json&>(const_cast<const json*>(this)->at(idx));
  }

  // value() with default
  template<typename T>
  T value(const std::string& key, T default_val) const {
    try {
      return at(key).get<T>();
    } catch (...) {
      return default_val;
    }
  }

  // ── 类型转换 ──
  template<typename T>
  T get() const;

  // ── 序列化 ──
  std::string dump(int indent = -1) const {
    std::ostringstream oss;
    dump_impl(oss, 0, indent);
    return oss.str();
  }

  // ── 大小 ──
  size_t size() const {
    if (type_ == json_type::array_) return arr_val_ ? arr_val_->size() : 0;
    if (type_ == json_type::object_) return obj_val_ ? obj_val_->size() : 0;
    return 0;
  }

  bool empty() const {
    return size() == 0;
  }

  // ── 迭代 ──
  using iterator = std::vector<json>::iterator;
  using const_iterator = std::vector<json>::const_iterator;

  iterator begin() {
    if (type_ != json_type::array_) throw type_error("not an array");
    return arr_val_->begin();
  }
  iterator end() {
    if (type_ != json_type::array_) throw type_error("not an array");
    return arr_val_->end();
  }
  const_iterator begin() const {
    if (type_ != json_type::array_) throw type_error("not an array");
    return arr_val_->begin();
  }
  const_iterator end() const {
    if (type_ != json_type::array_) throw type_error("not an array");
    return arr_val_->end();
  }

  // ── 对象迭代（用于 range-for） ──
  struct object_entry {
    std::string key;
    json value;
  };

  struct object_iterator {
    map_type::const_iterator it_;
    object_iterator(map_type::const_iterator it) : it_(it) {}
    object_entry operator*() const { return {it_->first, it_->second}; }
    bool operator!=(const object_iterator& o) const { return it_ != o.it_; }
    void operator++() { ++it_; }
  };

  object_iterator obegin() const {
    if (type_ != json_type::object_) throw type_error("not an object");
    return object_iterator(obj_val_->begin());
  }
  object_iterator oend() const {
    return object_iterator(obj_val_->end());
  }

private:
  // ── 类型定义 ──
  using map_type = std::map<std::string, json>;

  json_type type_ = json_type::null_;
  bool bool_val_ = false;
  int64_t int_val_ = 0;
  uint64_t uint_val_ = 0;
  double double_val_ = 0.0;
  std::string str_val_;
  std::vector<json> arr_val_;
  std::shared_ptr<map_type> obj_val_;

  // ── JSON 解析器 ──
  static void skip_ws(const std::string& s, size_t& pos) {
    while (pos < s.size() && (s[pos] == ' ' || s[pos] == '\t' ||
                              s[pos] == '\n' || s[pos] == '\r')) ++pos;
  }

  static json parse_value(const std::string& s, size_t& pos) {
    skip_ws(s, pos);
    if (pos >= s.size()) throw parse_error("unexpected end of input");
    if (s[pos] == '{') return parse_object(s, pos);
    if (s[pos] == '[') return parse_array(s, pos);
    if (s[pos] == '"') return parse_string(s, pos);
    if (s[pos] == 't' || s[pos] == 'f') return parse_bool(s, pos);
    if (s[pos] == 'n') return parse_null(s, pos);
    return parse_number(s, pos);
  }

  static json parse_object(const std::string& s, size_t& pos) {
    json obj = object();
    ++pos; // skip '{'
    skip_ws(s, pos);
    if (pos < s.size() && s[pos] == '}') { ++pos; return obj; }

    while (true) {
      skip_ws(s, pos);
      auto key = parse_string(s, pos).get<std::string>();
      skip_ws(s, pos);
      if (pos >= s.size() || s[pos] != ':') throw parse_error("expected ':'");
      ++pos; // skip ':'
      auto val = parse_value(s, pos);
      obj[key] = std::move(val);
      skip_ws(s, pos);
      if (pos < s.size() && s[pos] == ',') { ++pos; continue; }
      if (pos < s.size() && s[pos] == '}') { ++pos; break; }
      throw parse_error("expected ',' or '}' in object");
    }
    return obj;
  }

  static json parse_array(const std::string& s, size_t& pos) {
    json arr = array();
    ++pos; // skip '['
    skip_ws(s, pos);
    if (pos < s.size() && s[pos] == ']') { ++pos; return arr; }

    while (true) {
      arr.arr_val_->push_back(parse_value(s, pos));
      skip_ws(s, pos);
      if (pos < s.size() && s[pos] == ',') { ++pos; continue; }
      if (pos < s.size() && s[pos] == ']') { ++pos; break; }
      throw parse_error("expected ',' or ']' in array");
    }
    return arr;
  }

  static json parse_string(const std::string& s, size_t& pos) {
    if (pos >= s.size() || s[pos] != '"') throw parse_error("expected '\"'");
    ++pos; // skip opening '"'
    std::string result;
    while (pos < s.size() && s[pos] != '"') {
      if (s[pos] == '\\') {
        ++pos;
        if (pos >= s.size()) throw parse_error("unexpected end in string escape");
        switch (s[pos]) {
          case '"': result += '"'; break;
          case '\\': result += '\\'; break;
          case '/': result += '/'; break;
          case 'b': result += '\b'; break;
          case 'f': result += '\f'; break;
          case 'n': result += '\n'; break;
          case 'r': result += '\r'; break;
          case 't': result += '\t'; break;
          case 'u': {
            if (pos + 4 >= s.size()) throw parse_error("invalid unicode escape");
            result += "\\u" + s.substr(pos + 1, 4);
            pos += 4;
            break;
          }
          default: result += s[pos]; break;
        }
        ++pos;
      } else {
        result += s[pos];
        ++pos;
      }
    }
    if (pos >= s.size()) throw parse_error("unterminated string");
    ++pos; // skip closing '"'
    return json(std::move(result));
  }

  static json parse_number(const std::string& s, size_t& pos) {
    size_t start = pos;
    bool is_float = false;
    if (pos < s.size() && s[pos] == '-') ++pos;
    while (pos < s.size() && s[pos] >= '0' && s[pos] <= '9') ++pos;
    if (pos < s.size() && s[pos] == '.') { is_float = true; ++pos;
      while (pos < s.size() && s[pos] >= '0' && s[pos] <= '9') ++pos; }
    if (pos < s.size() && (s[pos] == 'e' || s[pos] == 'E')) { is_float = true; ++pos;
      if (pos < s.size() && (s[pos] == '+' || s[pos] == '-')) ++pos;
      while (pos < s.size() && s[pos] >= '0' && s[pos] <= '9') ++pos; }

    std::string num_str = s.substr(start, pos - start);

    if (is_float) {
      return json(std::stod(num_str));
    }
    // 尝试解析为 int64_t
    try {
      // 检查是否超出 int64_t 范围
      auto val = std::stoll(num_str);
      if (val >= 0) return json(static_cast<uint64_t>(val));
      return json(static_cast<int64_t>(val));
    } catch (...) {
      return json(std::stod(num_str));
    }
  }

  static json parse_bool(const std::string& s, size_t& pos) {
    if (s.substr(pos, 4) == "true") { pos += 4; return json(true); }
    if (s.substr(pos, 5) == "false") { pos += 5; return json(false); }
    throw parse_error("expected boolean");
  }

  static json parse_null(const std::string& s, size_t& pos) {
    if (s.substr(pos, 4) == "null") { pos += 4; return json(nullptr); }
    throw parse_error("expected null");
  }

  // ── 序列化 ──
  void dump_impl(std::ostream& os, int depth, int indent) const {
    switch (type_) {
      case json_type::null_:
        os << "null"; break;
      case json_type::bool_:
        os << (bool_val_ ? "true" : "false"); break;
      case json_type::int_:
        os << int_val_; break;
      case json_type::uint_:
        os << uint_val_; break;
      case json_type::double_:
        os << double_val_; break;
      case json_type::string_:
        os << '"';
        for (char c : str_val_) {
          switch (c) {
            case '"':  os << "\\\""; break;
            case '\\': os << "\\\\"; break;
            case '\b': os << "\\b"; break;
            case '\f': os << "\\f"; break;
            case '\n': os << "\\n"; break;
            case '\r': os << "\\r"; break;
            case '\t': os << "\\t"; break;
            default:
              if (static_cast<unsigned char>(c) < 0x20) {
                char buf[8];
                std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                os << buf;
              } else {
                os << c;
              }
          }
        }
        os << '"'; break;
      case json_type::array_:
        os << '[';
        if (arr_val_) {
          bool first = true;
          for (const auto& val : *arr_val_) {
            if (!first) os << ',';
            first = false;
            if (indent >= 0) os << '\n' << std::string(depth + 1, ' ');
            val.dump_impl(os, depth + 1, indent);
          }
          if (indent >= 0 && !arr_val_->empty())
            os << '\n' << std::string(depth, ' ');
        }
        os << ']'; break;
      case json_type::object_:
        os << '{';
        if (obj_val_) {
          bool first = true;
          for (const auto& [key, val] : *obj_val_) {
            if (!first) os << ',';
            first = false;
            if (indent >= 0) os << '\n' << std::string(depth + 1, ' ');
            os << '"' << key << "\":";
            if (indent >= 0) os << ' ';
            val.dump_impl(os, depth + 1, indent);
          }
          if (indent >= 0 && !obj_val_->empty())
            os << '\n' << std::string(depth, ' ');
        }
        os << '}'; break;
    }
  }
};

// ── 特化 get<T> ──
template<> inline std::string json::get<std::string>() const {
  if (type_ != json_type::string_) throw type_error("not a string");
  return str_val_;
}
template<> inline int json::get<int>() const {
  if (type_ == json_type::int_) return static_cast<int>(int_val_);
  if (type_ == json_type::uint_) return static_cast<int>(uint_val_);
  if (type_ == json_type::double_) return static_cast<int>(double_val_);
  throw type_error("not a number");
}
template<> inline unsigned json::get<unsigned>() const {
  if (type_ == json_type::uint_) return static_cast<unsigned>(uint_val_);
  if (type_ == json_type::int_) return static_cast<unsigned>(int_val_);
  throw type_error("not an unsigned number");
}
template<> inline int64_t json::get<int64_t>() const {
  if (type_ == json_type::int_) return int_val_;
  if (type_ == json_type::uint_) return static_cast<int64_t>(uint_val_);
  throw type_error("not an integer");
}
template<> inline uint64_t json::get<uint64_t>() const {
  if (type_ == json_type::uint_) return uint_val_;
  throw type_error("not an unsigned integer");
}
template<> inline double json::get<double>() const {
  if (type_ == json_type::double_) return double_val_;
  if (type_ == json_type::int_) return static_cast<double>(int_val_);
  if (type_ == json_type::uint_) return static_cast<double>(uint_val_);
  throw type_error("not a number");
}
template<> inline bool json::get<bool>() const {
  if (type_ != json_type::bool_) throw type_error("not a boolean");
  return bool_val_;
}

// ── >> 运算符 ──
inline std::istream& operator>>(std::istream& is, json& j) {
  std::stringstream buffer;
  buffer << is.rdbuf();
  j = json::parse(buffer.str());
  return is;
}

}  // namespace nlohmann

#endif  // NLOHMANN_JSON_MINIMAL_HPP
