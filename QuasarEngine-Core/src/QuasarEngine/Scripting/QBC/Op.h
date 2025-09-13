#pragma once

#include <cstdint>

namespace QuasarEngine::QBC
{
	enum Op : uint8_t
	{
		// Constantes / variables
		OP_CONST,
		OP_LOAD,
		OP_STORE,
		OP_POP,

		// Arith / logique
		OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_MOD,
		OP_EQ, OP_NE, OP_LT, OP_LE, OP_GT, OP_GE,
		OP_NOT,

		// Contrôle
		OP_JMP,
		OP_JMP_IF_FALSE,
		OP_HALT,

		// I/O pour tests
		OP_PRINT,

		// Fonctions (MVP: globales uniquement)
		OP_CALL,
		OP_RET
	};
}
