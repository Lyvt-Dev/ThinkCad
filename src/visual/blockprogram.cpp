#include "thinkercad/visual/blockprogram.hpp"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <regex>
#include <set>
#include <sstream>
#include <stdexcept>
namespace thinkercad::visual {
const std::vector<CommandSpec> &ArduinoCodeGenerator::commands() {
    using T = ValueType;
    static const std::vector<CommandSpec> values{{"pinMode", {T::Integer, T::Integer}, T::Void},
                                                 {"digitalWrite", {T::Integer, T::Integer}, T::Void},
                                                 {"digitalRead", {T::Integer}, T::Integer},
                                                 {"analogRead", {T::Integer}, T::Integer},
                                                 {"analogWrite", {T::Integer, T::Integer}, T::Void},
                                                 {"delay", {T::Integer}, T::Void},
                                                 {"millis", {}, T::Integer},
                                                 {"Serial.begin", {T::Integer}, T::Void},
                                                 {"Serial.print", {T::Text}, T::Void},
                                                 {"Serial.println", {T::Text}, T::Void},
                                                 {"Serial.available", {}, T::Integer},
                                                 {"Serial.read", {}, T::Integer},
                                                 {"Serial.write", {T::Integer}, T::Void},
                                                 {"servo.attach", {T::Integer}, T::Void},
                                                 {"servo.write", {T::Integer}, T::Void},
                                                 {"servo.writeMicroseconds", {T::Integer}, T::Void},
                                                 {"servo.detach", {}, T::Void},
                                                 {"Wire.begin", {}, T::Void},
                                                 {"Wire.beginTransmission", {T::Integer}, T::Void},
                                                 {"Wire.write", {T::Integer}, T::Integer},
                                                 {"Wire.endTransmission", {}, T::Integer},
                                                 {"Wire.requestFrom", {T::Integer, T::Integer}, T::Integer},
                                                 {"Wire.available", {}, T::Integer},
                                                 {"Wire.read", {}, T::Integer},
                                                 {"display.begin", {T::Integer, T::Integer}, T::Boolean},
                                                 {"display.clearDisplay", {}, T::Void},
                                                 {"display.setCursor", {T::Integer, T::Integer}, T::Void},
                                                 {"display.setTextSize", {T::Integer}, T::Void},
                                                 {"display.setTextColor", {T::Integer}, T::Void},
                                                 {"display.print", {T::Text}, T::Void},
                                                 {"display.println", {T::Text}, T::Void},
                                                 {"display.display", {}, T::Void}};
    return values;
}
namespace {
using T = ValueType;
const char *typeName(T t) {
    switch (t) {
    case T::Integer:
        return "int32_t";
    case T::Number:
        return "double";
    case T::Boolean:
        return "bool";
    case T::Text:
        return "String";
    case T::Void:
        return "void";
    }
    throw std::runtime_error("type");
}
bool identifier(const std::string &s) {
    static const std::regex pattern("[A-Za-z_][A-Za-z0-9_]*");
    static const std::set<std::string> reserved{"setup",
                                                "loop",
                                                "Serial",
                                                "Wire",
                                                "servo",
                                                "display",
                                                "int",
                                                "long",
                                                "bool",
                                                "double",
                                                "float",
                                                "char",
                                                "void",
                                                "String",
                                                "if",
                                                "else",
                                                "while",
                                                "for",
                                                "return",
                                                "class",
                                                "struct",
                                                "auto",
                                                "const",
                                                "true",
                                                "false",
                                                "HIGH",
                                                "LOW",
                                                "INPUT",
                                                "OUTPUT",
                                                "INPUT_PULLUP",
                                                "switch",
                                                "case",
                                                "break",
                                                "continue",
                                                "namespace",
                                                "using",
                                                "template",
                                                "delete",
                                                "new",
                                                "this",
                                                "sizeof",
                                                "static",
                                                "extern",
                                                "volatile",
                                                "do",
                                                "enum",
                                                "union",
                                                "public",
                                                "private",
                                                "protected",
                                                "operator",
                                                "throw",
                                                "try",
                                                "catch",
                                                "default",
                                                "unsigned",
                                                "signed",
                                                "short",
                                                "nullptr",
                                                "alignas",
                                                "alignof",
                                                "asm",
                                                "char8_t",
                                                "char16_t",
                                                "char32_t",
                                                "compl",
                                                "concept",
                                                "consteval",
                                                "constexpr",
                                                "constinit",
                                                "const_cast",
                                                "co_await",
                                                "co_return",
                                                "co_yield",
                                                "decltype",
                                                "dynamic_cast",
                                                "explicit",
                                                "export",
                                                "friend",
                                                "goto",
                                                "inline",
                                                "mutable",
                                                "noexcept",
                                                "not",
                                                "not_eq",
                                                "or",
                                                "or_eq",
                                                "register",
                                                "reinterpret_cast",
                                                "requires",
                                                "static_assert",
                                                "static_cast",
                                                "thread_local",
                                                "typeid",
                                                "typename",
                                                "virtual",
                                                "wchar_t",
                                                "xor",
                                                "xor_eq",
                                                "and",
                                                "and_eq",
                                                "bitand",
                                                "bitor"};
    return s.size() <= 64 && std::regex_match(s, pattern) && !reserved.contains(s) && !s.starts_with("__");
}
bool assignable(T expected, T actual) {
    return expected == actual || (expected == T::Number && (actual == T::Integer || actual == T::Boolean)) ||
           (expected == T::Integer && actual == T::Boolean);
}
struct GeneratedExpression {
    std::string text;
    T type;
};
class Generator {
    const BlockProgram &program_;
    const BoardProfile &board_;
    GenerationResult out_;
    std::map<std::string, T> symbols_;
    std::map<std::string, const Function *> functions_;
    std::set<std::string> ids_, features_;
    unsigned line_ = 1, nodes_ = 0;
    std::string current_;
    T returns_ = T::Void;
    void fail(const std::string &code) {
        throw std::runtime_error(code);
    }
    void count(unsigned depth) {
        if (++nodes_ > 4000 || depth > 64)
            fail("Program too complex");
    }
    void id(const std::string &value) {
        if (value.empty() || value == "setup" || value == "loop" || value.size() > 128 ||
            !ids_.insert(value).second)
            fail("Missing or duplicate block ID");
    }
    void emit(const std::string &text, unsigned indent = 0) {
        out_.lineBlocks[line_] = current_;
        out_.source += std::string(indent * 2, ' ') + text + '\n';
        ++line_;
    }
    void declare(const std::string &name, T type) {
        if (!identifier(name) || type == T::Void || symbols_.contains(name) || functions_.contains(name))
            fail("Invalid or duplicate variable");
        symbols_[name] = type;
    }
    static std::string quoted(const std::string &text) {
        std::string out = "\"";
        for (unsigned char c : text) {
            if (c == '\\' || c == '\"') {
                out += '\\';
                out += char(c);
            } else if (c == '\n')
                out += "\\n";
            else if (c == '\r')
                out += "\\r";
            else if (c == '\t')
                out += "\\t";
            else if (c < 32 || c == 127) {
                const char oct[] = "01234567";
                out += '\\';
                out += oct[(c >> 6) & 7];
                out += oct[(c >> 3) & 7];
                out += oct[c & 7];
            } else
                out += char(c);
        }
        return out + '\"';
    }
    static bool pure(const Expression &e) {
        if (e.kind == Expression::Kind::Call)
            return false;
        return std::all_of(e.arguments.begin(), e.arguments.end(), pure);
    }
    static bool returns(const std::vector<Block> &blocks) {
        if (blocks.empty())
            return false;
        const auto &last = blocks.back();
        return last.kind == Block::Kind::Return ||
               (last.kind == Block::Kind::If && returns(last.body) && returns(last.otherwise));
    }
    GeneratedExpression expression(const Expression &e, unsigned depth) {
        count(depth);
        using K = Expression::Kind;
        if (e.kind != K::Call && e.kind != K::Binary && !e.arguments.empty())
            fail("Unexpected expression arguments");
        if (e.kind == K::Number) {
            static const std::regex numeric("-?(0[xX][0-9a-fA-F]+|[0-9]+(\\.[0-9]+)?([eE][+-]?[0-9]+)?)");
            if (!std::regex_match(e.text, numeric) || e.text.size() > 32)
                fail("Invalid number");
            const bool hex = e.text.starts_with("0x") || e.text.starts_with("0X") ||
                             e.text.starts_with("-0x") || e.text.starts_with("-0X");
            if (hex || e.text.find_first_of(".eE") == std::string::npos) {
                bool negative = e.text.starts_with('-');
                auto digits = e.text.substr((negative ? 1 : 0) + (hex ? 2 : 0));
                unsigned long long n = 0;
                auto parsed = std::from_chars(digits.data(), digits.data() + digits.size(), n, hex ? 16 : 10);
                if (parsed.ec != std::errc() || n > (negative ? 2147483648ULL : 2147483647ULL))
                    fail("Integer out of range");
                return {(negative ? "-" : "") + std::to_string(n), T::Integer};
            }
            double value = 0;
            auto parsed = std::from_chars(e.text.data(), e.text.data() + e.text.size(), value);
            if (parsed.ec != std::errc() || !std::isfinite(value))
                fail("Number out of range");
            return {e.text, T::Number};
        }
        if (e.kind == K::Boolean) {
            if (e.text != "true" && e.text != "false")
                fail("Invalid boolean");
            return {e.text, T::Boolean};
        }
        if (e.kind == K::Text) {
            if (e.text.size() > 4096)
                fail("Text too long");
            return {quoted(e.text), T::Text};
        }
        if (e.kind == K::Constant) {
            static const std::set<std::string> constants{
                "HIGH",  "LOW",   "INPUT",         "OUTPUT",        "INPUT_PULLUP",
                "WHITE", "BLACK", "SSD1306_WHITE", "SSD1306_BLACK", "SSD1306_SWITCHCAPVCC"};
            if (!constants.contains(e.text))
                fail("Unknown constant");
            if (e.text.starts_with("SSD1306") || e.text == "WHITE" || e.text == "BLACK")
                features_.insert("oled");
            return {e.text, T::Integer};
        }
        if (e.kind == K::Variable) {
            if (!symbols_.contains(e.text))
                fail("Unknown variable");
            return {e.text, symbols_.at(e.text)};
        }
        if (e.kind == K::Call) {
            const CommandSpec *spec = nullptr;
            for (const auto &c : ArduinoCodeGenerator::commands())
                if (c.name == e.text) {
                    spec = &c;
                    break;
                }
            const Function *function = functions_.contains(e.text) ? functions_.at(e.text) : nullptr;
            if (!spec && !function)
                fail("Unknown function");
            const auto size = spec ? spec->arguments.size() : function->parameters.size();
            if (e.arguments.size() != size)
                fail("Incorrect argument count");
            std::string call = e.text + '(';
            for (std::size_t i = 0; i < size; ++i) {
                auto v = expression(e.arguments[i], depth + 1);
                T expected = spec ? spec->arguments[i] : function->parameters[i].type;
                const bool print = spec && (e.text.ends_with(".print") || e.text.ends_with(".println"));
                if ((!print && !assignable(expected, v.type)) || v.type == T::Void)
                    fail("Incorrect argument type");
                if (i)
                    call += ", ";
                call += v.text;
            }
            if (e.text.starts_with("servo."))
                features_.insert("servo");
            if (e.text.starts_with("Wire."))
                features_.insert("wire");
            if (e.text.starts_with("display."))
                features_.insert("oled");
            if (spec && !e.arguments.empty() &&
                (e.text == "pinMode" || e.text.starts_with("digital") || e.text.starts_with("analog") ||
                 e.text == "servo.attach")) {
                const auto &pin = e.arguments[0];
                if (pin.kind == K::Number && pin.text.find_first_not_of("0123456789") == std::string::npos) {
                    int n = std::stoi(pin.text);
                    const auto &pins = e.text == "analogRead" ? board_.adc : board_.gpio;
                    if (std::find(pins.begin(), pins.end(), n) == pins.end())
                        fail("Pin unavailable on selected board");
                    if (e.text == "analogWrite" && !board_.supportsPwm(n))
                        fail("PWM unavailable on selected pin");
                    if ((e.text == "digitalWrite" || e.text == "analogWrite" || e.text == "servo.attach") &&
                        std::find(board_.inputOnly.begin(), board_.inputOnly.end(), n) !=
                            board_.inputOnly.end())
                        fail("Pin is input only");
                }
            }
            return {call + ')', spec ? spec->result : function->result};
        }
        if (e.kind == K::Binary) {
            if (e.arguments.size() != 2)
                fail("Binary operation needs two arguments");
            auto a = expression(e.arguments[0], depth + 1), b = expression(e.arguments[1], depth + 1);
            const std::set<std::string> arithmetic{"+", "-", "*", "/", "%"},
                comparison{"==", "!=", "<", "<=", ">", ">="}, logical{"&&", "||"};
            if (a.type == T::Void || b.type == T::Void)
                fail("Void expression");
            T result = T::Boolean;
            if (arithmetic.contains(e.text)) {
                if (a.type == T::Text || b.type == T::Text)
                    fail("Numeric operation requires numbers");
                if (e.text == "%" && (a.type == T::Number || b.type == T::Number))
                    fail("Modulo requires integers");
                result = a.type == T::Number || b.type == T::Number ? T::Number : T::Integer;
            } else if (comparison.contains(e.text)) {
                if ((a.type == T::Text) != (b.type == T::Text))
                    fail("Incompatible comparison");
            } else if (logical.contains(e.text)) {
                if (a.type == T::Text || b.type == T::Text)
                    fail("Condition requires a number or boolean");
            } else
                fail("Unknown operator");
            return {'(' + a.text + ' ' + e.text + ' ' + b.text + ')', result};
        }
        fail("Unknown expression");
        return {};
    }
    void blocks(const std::vector<Block> &list, unsigned indent, unsigned depth) {
        if (depth > 64)
            fail("Program too complex");
        for (const auto &b : list) {
            count(depth);
            current_ = b.id;
            id(b.id);
            using K = Block::Kind;
            auto arg = [&](std::size_t n) {
                if (n >= b.arguments.size())
                    fail("Missing argument");
                return expression(b.arguments[n], depth + 1);
            };
            auto exact = [&](std::size_t n) {
                if (b.arguments.size() != n)
                    fail("Incorrect argument count");
            };
            auto scope = [&](const std::vector<Block> &body) {
                auto saved = symbols_;
                blocks(body, indent + 1, depth + 1);
                symbols_ = std::move(saved);
                current_ = b.id;
            };
            if (b.kind == K::Declare) {
                exact(1);
                auto v = arg(0);
                declare(b.name, b.type);
                if (!assignable(b.type, v.type))
                    fail("Incorrect initial value type");
                emit(std::string(typeName(b.type)) + " " + b.name + " = " + v.text + ";", indent);
            } else if (b.kind == K::Assign) {
                exact(1);
                auto v = arg(0);
                if (!symbols_.contains(b.name) || !assignable(symbols_.at(b.name), v.type))
                    fail("Invalid assignment");
                emit(b.name + " = " + v.text + ";", indent);
            } else if (b.kind == K::Call) {
                Expression e{Expression::Kind::Call, b.name, b.arguments};
                emit(expression(e, depth + 1).text + ";", indent);
            } else if (b.kind == K::Return) {
                if (returns_ == T::Void) {
                    exact(0);
                    emit("return;", indent);
                } else {
                    exact(1);
                    auto v = arg(0);
                    if (!assignable(returns_, v.type))
                        fail("Incorrect return type");
                    emit("return " + v.text + ";", indent);
                }
            } else if (b.kind == K::If || b.kind == K::While) {
                exact(1);
                auto v = arg(0);
                if (v.type == T::Void || v.type == T::Text)
                    fail("Invalid condition");
                emit(std::string(b.kind == K::If ? "if (" : "while (") + v.text + ") {", indent);
                scope(b.body);
                emit("}", indent);
                if (b.kind == K::If && !b.otherwise.empty()) {
                    emit("else {", indent);
                    scope(b.otherwise);
                    emit("}", indent);
                }
            } else if (b.kind == K::For) {
                exact(3);
                auto start = arg(0), end = arg(1), step = arg(2);
                if (start.type != T::Integer || end.type != T::Integer || step.type != T::Integer ||
                    b.arguments[2].kind != Expression::Kind::Number)
                    fail("For loop needs integer bounds and constant step");
                long long amount = std::stoll(step.text);
                if (!amount)
                    fail("For step cannot be zero");
                auto saved = symbols_;
                declare(b.name, T::Integer);
                emit("for (int32_t " + b.name + " = " + start.text + "; " + b.name +
                         (amount < 0 ? " > " : " < ") + end.text + "; " + b.name + " += " + step.text + ") {",
                     indent);
                blocks(b.body, indent + 1, depth + 1);
                symbols_ = std::move(saved);
                current_ = b.id;
                emit("}", indent);
            } else
                fail("Unknown block");
            if (b.kind != K::If && !b.otherwise.empty())
                fail("Else branch requires if");
            if (b.kind != K::If && b.kind != K::While && b.kind != K::For && !b.body.empty())
                fail("Unexpected nested blocks");
        }
    }

  public:
    Generator(const BlockProgram &p, const BoardProfile &board) : program_(p), board_(board) {}
    GenerationResult run() {
        try {
            if (program_.schema != 1)
                fail("Unsupported block schema");
            for (const auto &f : program_.functions) {
                count(0);
                current_ = f.id;
                id(f.id);
                if (f.result != T::Void && !returns(f.body))
                    fail("Function must return a value on all paths");
                if (!identifier(f.name) || !functions_.emplace(f.name, &f).second)
                    fail("Invalid or duplicate function");
            }
            emit("// Generated by ThinkCad visual coding. Edit blocks or switch to C++.");
            emit("#include <Arduino.h>");
            for (const auto &v : program_.globals) {
                auto e = expression(v.initial, 0);
                declare(v.name, v.type);
                if (!assignable(v.type, e.type) || !pure(v.initial))
                    fail("Invalid global initializer");
                emit(std::string(typeName(v.type)) + " " + v.name + " = " + e.text + ";");
            }
            const auto globals = symbols_;
            for (const auto &f : program_.functions) {
                current_ = f.id;
                symbols_ = globals;
                returns_ = f.result;
                std::string signature = std::string(typeName(f.result)) + " " + f.name + '(';
                for (std::size_t i = 0; i < f.parameters.size(); ++i) {
                    const auto &p = f.parameters[i];
                    declare(p.name, p.type);
                    if (i)
                        signature += ", ";
                    signature += std::string(typeName(p.type)) + " " + p.name;
                }
                emit(signature + ") {");
                blocks(f.body, 1, 0);
                emit("}");
            }
            for (bool setup : {true, false}) {
                symbols_ = globals;
                returns_ = T::Void;
                current_ = setup ? "setup" : "loop";
                emit(setup ? "void setup() {" : "void loop() {");
                blocks(setup ? program_.setup : program_.loop, 1, 0);
                emit("}");
            }
            std::string includes;
            if (features_.contains("wire") || features_.contains("oled"))
                includes += "#include <Wire.h>\n";
            if (features_.contains("servo"))
                includes += "#include <Servo.h>\nServo servo;\n";
            if (features_.contains("oled"))
                includes += "#include <Adafruit_GFX.h>\n#include <Adafruit_SSD1306.h>\nAdafruit_SSD1306 "
                            "display(128, 64, &Wire, -1);\n";
            // Function prototypes are explicit, independent of the Arduino preprocessor.
            for (const auto &f : program_.functions) {
                includes += std::string(typeName(f.result)) + " " + f.name + '(';
                for (std::size_t i = 0; i < f.parameters.size(); ++i) {
                    if (i)
                        includes += ", ";
                    includes += typeName(f.parameters[i].type);
                }
                includes += ");\n";
            }
            const auto offset = out_.source.find('\n', out_.source.find('\n') + 1) + 1;
            out_.source.insert(offset, includes);
            const auto added = unsigned(std::count(includes.begin(), includes.end(), '\n'));
            std::map<unsigned, std::string> map;
            for (auto [line, id] : out_.lineBlocks)
                map[line > 2 ? line + added : line] = id;
            out_.lineBlocks = std::move(map);
            if (out_.source.size() > 1024 * 1024)
                fail("Generated source too large");
        } catch (const std::exception &e) {
            out_.issues.push_back({current_, e.what()});
            out_.source.clear();
            out_.lineBlocks.clear();
        }
        return out_;
    }
};
} // namespace
GenerationResult ArduinoCodeGenerator::generate(const BlockProgram &p, const BoardProfile &board) {
    return Generator(p, board).run();
}
} // namespace thinkercad::visual
