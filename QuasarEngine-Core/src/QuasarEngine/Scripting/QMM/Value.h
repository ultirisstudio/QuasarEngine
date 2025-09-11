#pragma once

#include <string>
#include <variant>
#include <vector>
#include <functional>
#include <memory>
#include <sstream>
#include <iostream>

namespace QuasarEngine
{
    struct Function;
    using NativeFunction = std::function<std::variant<std::nullptr_t, double, std::string, bool>(const std::vector<std::variant<std::nullptr_t, double, std::string, bool>>&)>;

    struct Value {
        using Prim = std::variant<std::nullptr_t, double, std::string, bool>;
        std::variant<Prim, std::shared_ptr<Function>, std::shared_ptr<NativeFunction>> data;

        Value() : data(Prim{ std::nullptr_t{} }) {}
        explicit Value(Prim v) : data(std::move(v)) {}
        explicit Value(std::shared_ptr<Function> f) : data(std::move(f)) {}
        explicit Value(std::shared_ptr<NativeFunction> nf) : data(std::move(nf)) {}

        explicit Value(bool b) : data(Prim{ b }) {}
        explicit Value(double d) : data(Prim{ d }) {}
        explicit Value(const std::string& s) : data(Prim{ s }) {}
        explicit Value(const char* s) : data(Prim{ std::string{s} }) {}
        explicit Value(std::nullptr_t) : data(Prim{ std::nullptr_t{} }) {}

        bool isPrim() const { return std::holds_alternative<Prim>(data); }
        Prim& prim() { return std::get<Prim>(data); }
        const Prim& prim() const { return std::get<Prim>(data); }
    };

    static std::string toString(const Value& v) {
        if (std::holds_alternative<Value::Prim>(v.data)) {
            const auto& p = std::get<Value::Prim>(v.data);
            if (std::holds_alternative<std::nullptr_t>(p)) return "nil";
            if (std::holds_alternative<double>(p)) { std::ostringstream oss; oss << std::get<double>(p); return oss.str(); }
            if (std::holds_alternative<std::string>(p)) return std::get<std::string>(p);
            if (std::holds_alternative<bool>(p)) return std::get<bool>(p) ? "true" : "false";
        }
        if (std::holds_alternative<std::shared_ptr<Function>>(v.data)) return "<fn>";
        return "<native>";
    }

    static bool primEqual(const Value::Prim& a, const Value::Prim& b) {
        if (a.index() != b.index()) return false;
        if (std::holds_alternative<std::nullptr_t>(a)) return true;
        if (std::holds_alternative<double>(a)) return std::get<double>(a) == std::get<double>(b);
        if (std::holds_alternative<std::string>(a)) return std::get<std::string>(a) == std::get<std::string>(b);
        if (std::holds_alternative<bool>(a)) return std::get<bool>(a) == std::get<bool>(b);
        return false;
    }

    static bool valueEqual(const Value& a, const Value& b) {
        if (a.isPrim() && b.isPrim()) return primEqual(a.prim(), b.prim());
        if (auto fa = std::get_if<std::shared_ptr<Function>>(&a.data))
            if (auto fb = std::get_if<std::shared_ptr<Function>>(&b.data)) return fa->get() == fb->get();
        if (auto na = std::get_if<std::shared_ptr<NativeFunction>>(&a.data))
            if (auto nb = std::get_if<std::shared_ptr<NativeFunction>>(&b.data)) return na->get() == nb->get();
        return false;
    }
}