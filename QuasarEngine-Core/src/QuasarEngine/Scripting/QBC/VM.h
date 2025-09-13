#pragma once

#include "Op.h"
#include "Chunk.h"

#include <unordered_map>
#include <iostream>
#include <functional>
#include <variant>
#include <cmath>
#include <sstream>
#include <stdexcept>

namespace QuasarEngine::QBC
{
    struct Value {
        using Prim = std::variant<std::nullptr_t, double, std::string, bool>;
        Prim prim;
    };

    inline bool truthy(const Value& v) {
        if (std::holds_alternative<std::nullptr_t>(v.prim)) return false;
        if (auto b = std::get_if<bool>(&v.prim)) return *b;
        if (auto d = std::get_if<double>(&v.prim)) return *d != 0.0;
        if (auto s = std::get_if<std::string>(&v.prim)) return !s->empty();
        return true;
    }
    inline std::string to_string(const Value& v) {
        if (std::holds_alternative<std::nullptr_t>(v.prim)) return "nil";
        if (auto b = std::get_if<bool>(&v.prim)) return *b ? "true" : "false";
        if (auto d = std::get_if<double>(&v.prim)) { std::ostringstream o; o << *d; return o.str(); }
        if (auto s = std::get_if<std::string>(&v.prim)) return *s;
        return "<v>";
    }

    using Native = std::function<Value(const std::vector<Value>&)>;

    struct VM {
        std::unordered_map<std::string, Value> globals;
        std::unordered_map<std::string, Native> natives;

        void registerNative(const std::string& name, Native fn) { natives[name] = std::move(fn); }

        static Value fromConst(const Const& c) {
            Value v;
            if (std::holds_alternative<double>(c)) v.prim = std::get<double>(c);
            else if (std::holds_alternative<std::string>(c)) v.prim = std::get<std::string>(c);
            else if (std::holds_alternative<bool>(c)) v.prim = std::get<bool>(c);
            else v.prim = nullptr;
            return v;
        }

        Value run(const Chunk& chunk) {
            std::vector<Value> stack;
            size_t ip = 0;

            auto pop = [&]() { Value v = stack.back(); stack.pop_back(); return v; };
            auto push = [&](const Value& v) { stack.push_back(v); };

            while (ip < chunk.code.size()) {
                Op op = (Op)chunk.code[ip++];

                switch (op) {
                case OP_CONST: {
                    uint32_t id = read_u32(chunk.code, ip);
                    push(fromConst(chunk.constants[id]));
                } break;
                case OP_LOAD: {
                    uint32_t id = read_u32(chunk.code, ip);
                    auto& name = chunk.names[id];
                    push(globals[name]); // défaut = nil
                } break;
                case OP_STORE: {
                    uint32_t id = read_u32(chunk.code, ip);
                    auto& name = chunk.names[id];
                    globals[name] = pop();
                } break;
                case OP_POP: { (void)pop(); } break;

                case OP_ADD: { auto b = pop(), a = pop(); push(Value{ std::get<double>(a.prim) + std::get<double>(b.prim) }); } break;
                case OP_SUB: { auto b = pop(), a = pop(); push(Value{ std::get<double>(a.prim) - std::get<double>(b.prim) }); } break;
                case OP_MUL: { auto b = pop(), a = pop(); push(Value{ std::get<double>(a.prim) * std::get<double>(b.prim) }); } break;
                case OP_DIV: { auto b = pop(), a = pop(); double d = std::get<double>(b.prim); if (d == 0.0) throw std::runtime_error("div0"); push(Value{ std::get<double>(a.prim) / d }); } break;
                case OP_MOD: { auto b = pop(), a = pop(); double d = std::get<double>(b.prim); if (d == 0.0) throw std::runtime_error("mod0"); push(Value{ std::fmod(std::get<double>(a.prim), d) }); } break;

                case OP_EQ: { auto b = pop(), a = pop(); push(Value{ (double)(to_string(a) == to_string(b)) }); } break;
                case OP_NE: { auto b = pop(), a = pop(); push(Value{ (double)(to_string(a) != to_string(b)) }); } break;
                case OP_LT: { auto b = pop(), a = pop(); push(Value{ std::get<double>(a.prim) < std::get<double>(b.prim) }); } break;
                case OP_LE: { auto b = pop(), a = pop(); push(Value{ std::get<double>(a.prim) <= std::get<double>(b.prim) }); } break;
                case OP_GT: { auto b = pop(), a = pop(); push(Value{ std::get<double>(a.prim) > std::get<double>(b.prim) }); } break;
                case OP_GE: { auto b = pop(), a = pop(); push(Value{ std::get<double>(a.prim) >= std::get<double>(b.prim) }); } break;
                case OP_NOT: { auto a = pop(); push(Value{ (double)!truthy(a) }); } break;

                case OP_JMP: {
                    uint32_t addr = read_u32(chunk.code, ip);
                    ip = addr;
                } break;
                case OP_JMP_IF_FALSE: {
                    uint32_t addr = read_u32(chunk.code, ip);
                    auto cond = pop();
                    if (!truthy(cond)) ip = addr;
                } break;

                case OP_PRINT: {
                    auto v = pop();
                    std::cout << to_string(v) << "\n";
                } break;

                case OP_CALL: {
                    uint32_t name_id = read_u32(chunk.code, ip);
                    uint8_t  argc = chunk.code[ip++];
                    auto& fname = chunk.names[name_id];
                    std::vector<Value> args(argc);
                    for (int i = argc - 1; i >= 0; --i) args[i] = pop();
                    auto it = natives.find(fname);
                    if (it == natives.end()) throw std::runtime_error("native not found: " + fname);
                    push(it->second(args));
                } break;

                case OP_RET: {
                    return stack.empty() ? Value{ nullptr } : pop();
                }
                case OP_HALT:
                    return stack.empty() ? Value{ nullptr } : pop();
                }
            }
            return Value{ nullptr };
        }
    };
}
