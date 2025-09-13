#pragma once

#include "Chunk.h"
#include "Op.h"
#include <unordered_map>
#include <memory>

namespace QuasarEngine
{
    struct Expr; struct Stmt;
    struct LiteralExpr; struct VariableExpr; struct AssignExpr; struct UnaryExpr; struct BinaryExpr;
    struct CallExpr; struct ExprStmt; struct LetStmt; struct BlockStmt; struct IfStmt; struct WhileStmt;
    using ExprPtr = std::shared_ptr<Expr>;
    using StmtPtr = std::shared_ptr<Stmt>;
}

namespace QuasarEngine::QBC
{
    struct Compiler {
        Chunk chunk;
        std::unordered_map<std::string, uint32_t> nameIds;

        uint32_t nameId(const std::string& s) {
            auto it = nameIds.find(s);
            if (it != nameIds.end()) return it->second;
            uint32_t id = add_name(chunk, s);
            nameIds[s] = id;
            return id;
        }

        size_t emitJump(Op op) {
            emit_u8(chunk, (uint8_t)op);
            size_t pos = chunk.code.size();
            emit_u32(chunk, 0xFFFFFFFF);
            return pos;
        }

        void patchJump(size_t at) {
            uint32_t addr = (uint32_t)chunk.code.size();
            chunk.code[at + 0] = (addr >> 0) & 0xFF;
            chunk.code[at + 1] = (addr >> 8) & 0xFF;
            chunk.code[at + 2] = (addr >> 16) & 0xFF;
            chunk.code[at + 3] = (addr >> 24) & 0xFF;
        }

        void emitStmt(const QuasarEngine::StmtPtr& s);
        void emitExpr(const QuasarEngine::ExprPtr& e);
    };
}
