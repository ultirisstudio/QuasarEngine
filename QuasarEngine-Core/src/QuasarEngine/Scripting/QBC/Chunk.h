#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <variant>
#include <unordered_map>

namespace QuasarEngine::QBC
{
	using Const = std::variant<std::nullptr_t, double, std::string, bool>;

	struct Chunk {
		std::vector<uint8_t> code;
		std::vector<Const>   constants;
		std::vector<std::string> names;
	};

	inline void emit_u8(Chunk& c, uint8_t b) { c.code.push_back(b); }
	inline void emit_u32(Chunk& c, uint32_t v) {
		for (int i = 0; i < 4; i++) c.code.push_back((v >> (i * 8)) & 0xFF);
	}
	inline uint32_t read_u32(const std::vector<uint8_t>& code, size_t& ip) {
		uint32_t v = 0; for (int i = 0; i < 4; i++) v |= (uint32_t)code[ip++] << (i * 8); return v;
	}
	inline uint32_t add_const(Chunk& c, Const k) { c.constants.push_back(k); return (uint32_t)c.constants.size() - 1; }
	inline uint32_t add_name(Chunk& c, std::string s) { c.names.push_back(std::move(s)); return (uint32_t)c.names.size() - 1; }
}
