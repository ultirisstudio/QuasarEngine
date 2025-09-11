#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "Value.h"
#include "AST.h"
#include "Error.h"

namespace QuasarEngine {
    struct Environment : std::enable_shared_from_this<Environment> {
        std::unordered_map<std::string, Value> values;
        std::shared_ptr<Environment> enclosing;
        explicit Environment(std::shared_ptr<Environment> parent = nullptr) :enclosing(std::move(parent)) {}
        void define(const std::string& name, Value v) { values[name] = std::move(v); }
        bool assign(const std::string& name, Value v) {
            if (values.count(name)) { values[name] = std::move(v); return true; }
            if (enclosing) return enclosing->assign(name, std::move(v));
            return false;
        }
        Value get(const std::string& name) {
            if (values.count(name)) return values[name];
            if (enclosing) return enclosing->get(name);
            throw QError({ 1,1 }, std::string("Variable non définie: ") + name);
        }
    };

    struct Function {
        std::vector<std::string> params; std::shared_ptr<BlockStmt> body; std::shared_ptr<Environment> closure;
        Function(std::vector<std::string> p, std::shared_ptr<BlockStmt> b, std::shared_ptr<Environment> e)
            : params(std::move(p)), body(std::move(b)), closure(std::move(e)) {
        }
    };

    struct ReturnJump { Value value; };
}
