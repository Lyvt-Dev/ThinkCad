#include "thinkercad/visual/expressionparser.hpp"
#include <cctype>
#include <stdexcept>
#include <set>
namespace thinkercad::visual {
namespace {
class Parser {
    std::string_view input_;
    std::size_t pos_ = 0;
    unsigned nodes_ = 0;
    [[noreturn]] void fail() {
        throw std::runtime_error("Invalid expression");
    }
    void space() {
        while (pos_ < input_.size() && std::isspace(static_cast<unsigned char>(input_[pos_])))
            ++pos_;
    }
    bool take(char c) {
        space();
        if (pos_ < input_.size() && input_[pos_] == c) {
            ++pos_;
            return true;
        }
        return false;
    }
    static int precedence(const std::string &op) {
        if (op == "||")
            return 1;
        if (op == "&&")
            return 2;
        if (op == "==" || op == "!=")
            return 3;
        if (op == "<" || op == "<=" || op == ">" || op == ">=")
            return 4;
        if (op == "+" || op == "-")
            return 5;
        if (op == "*" || op == "/" || op == "%")
            return 6;
        return 0;
    }
    Expression primary(unsigned depth) {
        if (depth > 64 || ++nodes_ > 512)
            fail();
        space();
        if (pos_ == input_.size())
            fail();
        if (take('(')) {
            auto v = binary(1, depth + 1);
            if (!take(')'))
                fail();
            return v;
        }
        if (take('-')) {
            auto value = primary(depth + 1);
            if (value.kind == Expression::Kind::Number) {
                value.text = "-" + value.text;
                return value;
            }
            return {Expression::Kind::Binary, "-", {{Expression::Kind::Number, "0", {}}, std::move(value)}};
        }
        if (take('!'))
            return {Expression::Kind::Binary,
                    "==",
                    {primary(depth + 1), {Expression::Kind::Boolean, "false", {}}}};
        if (take('"')) {
            std::string value;
            bool closed = false;
            while (pos_ < input_.size()) {
                char c = input_[pos_++];
                if (c == '"') {
                    closed = true;
                    break;
                }
                if (c == '\\') {
                    if (pos_ == input_.size())
                        fail();
                    c = input_[pos_++];
                    if (c == 'n')
                        c = '\n';
                    else if (c == 'r')
                        c = '\r';
                    else if (c == 't')
                        c = '\t';
                    else if (c != '"' && c != '\\')
                        fail();
                }
                value += c;
            }
            if (!closed)
                fail();
            return {Expression::Kind::Text, value, {}};
        }
        const auto begin = pos_;
        if (std::isdigit(static_cast<unsigned char>(input_[pos_]))) {
            if (input_.substr(pos_, 2) == "0x" || input_.substr(pos_, 2) == "0X") {
                pos_ += 2;
                while (pos_ < input_.size() && std::isxdigit(static_cast<unsigned char>(input_[pos_])))
                    ++pos_;
            } else {
                while (pos_ < input_.size() &&
                       (std::isdigit(static_cast<unsigned char>(input_[pos_])) || input_[pos_] == '.'))
                    ++pos_;
                if (pos_ < input_.size() && (input_[pos_] == 'e' || input_[pos_] == 'E')) {
                    ++pos_;
                    if (pos_ < input_.size() && (input_[pos_] == '+' || input_[pos_] == '-'))
                        ++pos_;
                    while (pos_ < input_.size() && std::isdigit(static_cast<unsigned char>(input_[pos_])))
                        ++pos_;
                }
            }
            return {Expression::Kind::Number, std::string(input_.substr(begin, pos_ - begin)), {}};
        }
        if (!std::isalpha(static_cast<unsigned char>(input_[pos_])) && input_[pos_] != '_')
            fail();
        while (pos_ < input_.size() && (std::isalnum(static_cast<unsigned char>(input_[pos_])) ||
                                        input_[pos_] == '_' || input_[pos_] == '.'))
            ++pos_;
        auto name = std::string(input_.substr(begin, pos_ - begin));
        if (take('(')) {
            Expression call{Expression::Kind::Call, name, {}};
            if (!take(')')) {
                do {
                    call.arguments.push_back(binary(1, depth + 1));
                } while (take(','));
                if (!take(')'))
                    fail();
            }
            return call;
        }
        if (name == "true" || name == "false")
            return {Expression::Kind::Boolean, name, {}};
        static const std::set<std::string> constants{
            "HIGH",  "LOW",   "INPUT",         "OUTPUT",        "INPUT_PULLUP",
            "WHITE", "BLACK", "SSD1306_WHITE", "SSD1306_BLACK", "SSD1306_SWITCHCAPVCC"};
        return {constants.contains(name) ? Expression::Kind::Constant : Expression::Kind::Variable, name, {}};
    }
    Expression binary(int min, unsigned depth) {
        auto left = primary(depth + 1);
        while (true) {
            space();
            auto start = pos_;
            std::string op;
            if (pos_ + 1 < input_.size() && precedence(std::string(input_.substr(pos_, 2)))) {
                op = input_.substr(pos_, 2);
                pos_ += 2;
            } else if (pos_ < input_.size() && precedence(std::string(1, input_[pos_]))) {
                op = input_.substr(pos_, 1);
                ++pos_;
            }
            int p = precedence(op);
            if (p < min) {
                pos_ = start;
                break;
            }
            auto right = binary(p + 1, depth + 1);
            left = {Expression::Kind::Binary, op, {std::move(left), std::move(right)}};
            if (++nodes_ > 512)
                fail();
        }
        return left;
    }

  public:
    explicit Parser(std::string_view in) : input_(in) {
        if (in.size() > 8192)
            fail();
    }
    Expression run() {
        auto v = binary(1, 0);
        space();
        if (pos_ != input_.size())
            fail();
        return v;
    }
};
} // namespace
Expression ExpressionParser::parse(std::string_view text) {
    return Parser(text).run();
}
} // namespace thinkercad::visual
