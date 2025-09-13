#include "qepch.h"

#include "Compiler.h"
#include "Op.h"
#include "Chunk.h"

#include <QuasarEngine/Scripting/QMM/Interpreter.h>
#include <QuasarEngine/Scripting/QMM/Parser.h>
#include <QuasarEngine/Scripting/QMM/Lexer.h>

#include <stdexcept>

namespace QuasarEngine::QBC
{
    void Compiler::emitExpr(const ExprPtr& e) {
        if (auto lit = std::dynamic_pointer_cast<LiteralExpr>(e)) {
            uint32_t k = add_const(chunk, lit->value);
            emit_u8(chunk, (uint8_t)OP_CONST);
            emit_u32(chunk, k);
            return;
        }
        if (auto var = std::dynamic_pointer_cast<VariableExpr>(e)) {
            uint32_t id = nameId(var->name);
            emit_u8(chunk, (uint8_t)OP_LOAD);
            emit_u32(chunk, id);
            return;
        }
        if (auto asg = std::dynamic_pointer_cast<AssignExpr>(e)) {
            emitExpr(asg->value);
            uint32_t id = nameId(asg->name);
            emit_u8(chunk, (uint8_t)OP_STORE);
            emit_u32(chunk, id);
            emit_u8(chunk, (uint8_t)OP_LOAD);
            emit_u32(chunk, id);
            return;
        }
        if (auto un = std::dynamic_pointer_cast<UnaryExpr>(e)) {
            emitExpr(un->right);
            if (un->op == TokenType::BANG) { emit_u8(chunk, (uint8_t)OP_NOT); }
            else if (un->op == TokenType::MINUS) {
                uint32_t k = add_const(chunk, 0.0);
                emit_u8(chunk, (uint8_t)OP_CONST); emit_u32(chunk, k);
                //emit_u8(chunk, (uint8_t)OP_SWAP);
                emit_u8(chunk, (uint8_t)OP_SUB);
            }
            return;
        }
        if (auto bin = std::dynamic_pointer_cast<BinaryExpr>(e)) {
            emitExpr(bin->left);
            emitExpr(bin->right);
            switch (bin->op) {
            case TokenType::PLUS: emit_u8(chunk, (uint8_t)OP_ADD); break;
            case TokenType::MINUS: emit_u8(chunk, (uint8_t)OP_SUB); break;
            case TokenType::STAR: emit_u8(chunk, (uint8_t)OP_MUL); break;
            case TokenType::SLASH: emit_u8(chunk, (uint8_t)OP_DIV); break;
            case TokenType::PERCENT: emit_u8(chunk, (uint8_t)OP_MOD); break;
            case TokenType::EQUAL_EQUAL: emit_u8(chunk, (uint8_t)OP_EQ); break;
            case TokenType::BANG_EQUAL: emit_u8(chunk, (uint8_t)OP_NE); break;
            case TokenType::LESS: emit_u8(chunk, (uint8_t)OP_LT); break;
            case TokenType::LESS_EQUAL: emit_u8(chunk, (uint8_t)OP_LE); break;
            case TokenType::GREATER: emit_u8(chunk, (uint8_t)OP_GT); break;
            case TokenType::GREATER_EQUAL: emit_u8(chunk, (uint8_t)OP_GE); break;
            default: throw std::runtime_error("bin op not supported in BC MVP");
            }
            return;
        }
        if (auto call = std::dynamic_pointer_cast<CallExpr>(e)) {
            auto calleeVar = std::dynamic_pointer_cast<VariableExpr>(call->callee);
            if (!calleeVar) throw std::runtime_error("MVP bytecode: call expects global name");
            for (auto& arg : call->args) emitExpr(arg);
            emit_u8(chunk, (uint8_t)OP_CALL);
            emit_u32(chunk, nameId(calleeVar->name));
            emit_u8(chunk, (uint8_t)call->args.size());
            return;
        }
        throw std::runtime_error("expr not supported in BC MVP");
    }

    void Compiler::emitStmt(const StmtPtr& s) {
        if (auto es = std::dynamic_pointer_cast<ExprStmt>(s)) {
            emitExpr(es->expr);
            emit_u8(chunk, (uint8_t)OP_POP);
            return;
        }
        if (auto ls = std::dynamic_pointer_cast<LetStmt>(s)) {
            if (ls->initializer) emitExpr(ls->initializer);
            else { uint32_t k = add_const(chunk, nullptr); emit_u8(chunk, (uint8_t)OP_CONST); emit_u32(chunk, k); }
            uint32_t id = nameId(ls->name);
            emit_u8(chunk, (uint8_t)OP_STORE); emit_u32(chunk, id);
            return;
        }
        if (auto bs = std::dynamic_pointer_cast<BlockStmt>(s)) {
            for (auto& st : bs->statements) emitStmt(st);
            return;
        }
        if (auto is = std::dynamic_pointer_cast<IfStmt>(s)) {
            emitExpr(is->cond);
            size_t jmpFalse = emitJump(OP_JMP_IF_FALSE);
            emit_u8(chunk, (uint8_t)OP_POP);
            emitStmt(is->thenBranch);
            size_t jmpEnd = emitJump(OP_JMP);
            patchJump(jmpFalse);
            emit_u8(chunk, (uint8_t)OP_POP);
            if (is->elseBranch) emitStmt(is->elseBranch);
            patchJump(jmpEnd);
            return;
        }
        if (auto ws = std::dynamic_pointer_cast<WhileStmt>(s)) {
            size_t loopStart = chunk.code.size();
            emitExpr(ws->cond);
            size_t jmpFalse = emitJump(OP_JMP_IF_FALSE);
            emit_u8(chunk, (uint8_t)OP_POP);
            emitStmt(ws->body);
            emit_u8(chunk, (uint8_t)OP_JMP); emit_u32(chunk, (uint32_t)loopStart);
            patchJump(jmpFalse);
            emit_u8(chunk, (uint8_t)OP_POP);
            return;
        }
        throw std::runtime_error("stmt not supported in BC MVP");
    }
}