#pragma once
#include "thinkercad/simulation.hpp"
#include <memory>
#include <optional>
namespace thinkercad::visual {
enum class ValueType { Integer, Number, Boolean, Text, Void };
struct Expression {
    enum class Kind { Number, Boolean, Text, Variable, Binary, Call, Constant };
    Kind kind = Kind::Number;
    std::string text = "0";
    std::vector<Expression> arguments;
};
struct Variable {
    std::string name;
    ValueType type = ValueType::Integer;
    Expression initial;
};
struct Block {
    enum class Kind { Declare, Assign, If, While, For, Call, Return };
    std::string id;
    Kind kind = Kind::Call;
    std::string name;
    ValueType type = ValueType::Integer;
    std::vector<Expression> arguments;
    std::vector<Block> body, otherwise;
};
struct Function {
    std::string id, name;
    ValueType result = ValueType::Void;
    std::vector<Variable> parameters;
    std::vector<Block> body;
};
struct BlockProgram {
    unsigned schema = 1;
    std::vector<Variable> globals;
    std::vector<Function> functions;
    std::vector<Block> setup, loop;
};
struct GenerationIssue {
    std::string block, code;
};
struct GenerationResult {
    std::string source;
    std::map<unsigned, std::string> lineBlocks;
    std::vector<GenerationIssue> issues;
    bool valid() const {
        return issues.empty();
    }
};
struct CommandSpec {
    std::string name;
    std::vector<ValueType> arguments;
    ValueType result;
};
class ArduinoCodeGenerator {
  public:
    static constexpr unsigned version = 1;
    static GenerationResult generate(const BlockProgram &, const BoardProfile &);
    static const std::vector<CommandSpec> &commands();
};
} // namespace thinkercad::visual
