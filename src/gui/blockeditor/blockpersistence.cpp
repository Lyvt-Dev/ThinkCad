#include "blockpersistence.hpp"
#include <QJsonArray>
#include <QJsonDocument>
#include <QCryptographicHash>
#include <stdexcept>
using namespace thinkercad::visual;
namespace {
void need(bool condition) {
    if (!condition)
        throw std::runtime_error("Invalid visual program");
}
QString text(const QJsonObject &o, const char *key) {
    need(o[key].isString());
    const auto s = o[key].toString();
    need(s.size() < 8192);
    return s;
}
int integer(const QJsonObject &o, const char *key, int max) {
    need(o[key].isDouble());
    int n = o[key].toInt(-1);
    need(n >= 0 && n <= max && o[key].toDouble() == n);
    return n;
}
struct Reader {
    int count = 0;
    void limit(int depth) {
        need(depth <= 64 && ++count <= 4000);
    }
    Expression expression(const QJsonObject &o, int depth) {
        limit(depth);
        Expression e;
        e.kind = Expression::Kind(integer(o, "kind", 6));
        e.text = text(o, "text").toStdString();
        need(o["arguments"].isArray());
        for (auto v : o["arguments"].toArray()) {
            need(v.isObject());
            e.arguments.push_back(expression(v.toObject(), depth + 1));
        }
        return e;
    }
    Variable variable(const QJsonObject &o, int depth) {
        limit(depth);
        need(o["initial"].isObject());
        return {text(o, "name").toStdString(), ValueType(integer(o, "type", 3)),
                expression(o["initial"].toObject(), depth + 1)};
    }
    std::vector<Block> blocks(const QJsonArray &array, int depth) {
        std::vector<Block> result;
        for (auto v : array) {
            limit(depth);
            need(v.isObject());
            auto o = v.toObject();
            Block b;
            b.id = text(o, "id").toStdString();
            b.name = text(o, "name").toStdString();
            b.kind = Block::Kind(integer(o, "kind", 6));
            b.type = ValueType(integer(o, "type", 4));
            for (const auto *key : {"arguments", "body", "otherwise"})
                need(o[key].isArray());
            for (auto a : o["arguments"].toArray()) {
                need(a.isObject());
                b.arguments.push_back(expression(a.toObject(), depth + 1));
            }
            b.body = blocks(o["body"].toArray(), depth + 1);
            b.otherwise = blocks(o["otherwise"].toArray(), depth + 1);
            result.push_back(std::move(b));
        }
        return result;
    }
};
QJsonObject expr(const Expression &e) {
    QJsonArray args;
    for (const auto &a : e.arguments)
        args.append(expr(a));
    return {{"kind", int(e.kind)}, {"text", QString::fromStdString(e.text)}, {"arguments", args}};
}
QJsonObject var(const Variable &v) {
    return {{"name", QString::fromStdString(v.name)}, {"type", int(v.type)}, {"initial", expr(v.initial)}};
}
QJsonArray blocks(const std::vector<Block> &list) {
    QJsonArray array;
    for (const auto &b : list) {
        QJsonArray args;
        for (const auto &a : b.arguments)
            args.append(expr(a));
        array.append(QJsonObject{{"id", QString::fromStdString(b.id)},
                                 {"kind", int(b.kind)},
                                 {"name", QString::fromStdString(b.name)},
                                 {"type", int(b.type)},
                                 {"arguments", args},
                                 {"body", blocks(b.body)},
                                 {"otherwise", blocks(b.otherwise)}});
    }
    return array;
}
} // namespace
namespace BlockPersistence {
QJsonObject encode(const BlockProgram &p) {
    QJsonArray globals, functions;
    for (const auto &v : p.globals)
        globals.append(var(v));
    for (const auto &f : p.functions) {
        QJsonArray args;
        for (const auto &v : f.parameters)
            args.append(var(v));
        functions.append(QJsonObject{{"id", QString::fromStdString(f.id)},
                                     {"name", QString::fromStdString(f.name)},
                                     {"result", int(f.result)},
                                     {"parameters", args},
                                     {"body", blocks(f.body)}});
    }
    return {{"schema", int(p.schema)},
            {"globals", globals},
            {"functions", functions},
            {"setup", blocks(p.setup)},
            {"loop", blocks(p.loop)}};
}
BlockProgram decode(const QJsonObject &o) {
    need(QJsonDocument(o).toJson(QJsonDocument::Compact).size() <= 1024 * 1024);
    need(integer(o, "schema", 1) == 1);
    for (const auto *key : {"globals", "functions", "setup", "loop"})
        need(o[key].isArray());
    Reader r;
    BlockProgram p;
    for (auto v : o["globals"].toArray()) {
        need(v.isObject());
        p.globals.push_back(r.variable(v.toObject(), 0));
    }
    for (auto v : o["functions"].toArray()) {
        r.limit(0);
        need(v.isObject());
        auto f = v.toObject();
        Function fun;
        fun.id = text(f, "id").toStdString();
        fun.name = text(f, "name").toStdString();
        fun.result = ValueType(integer(f, "result", 4));
        need(f["parameters"].isArray() && f["body"].isArray());
        for (auto a : f["parameters"].toArray()) {
            need(a.isObject());
            fun.parameters.push_back(r.variable(a.toObject(), 1));
        }
        fun.body = r.blocks(f["body"].toArray(), 1);
        p.functions.push_back(std::move(fun));
    }
    p.setup = r.blocks(o["setup"].toArray(), 0);
    p.loop = r.blocks(o["loop"].toArray(), 0);
    return p;
}
QString fingerprint(const QString &source) {
    return QString::fromLatin1(QCryptographicHash::hash(source.toUtf8(), QCryptographicHash::Sha256).toHex());
}
} // namespace BlockPersistence
