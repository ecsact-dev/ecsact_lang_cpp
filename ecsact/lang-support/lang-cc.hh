#pragma once

#include <string>
#include "ecsact/runtime/definitions.h"

namespace ecsact::cc_lang_support {

/**
 * Convert strings like foo.bar to foo::bar
 */
inline std::string cpp_identifier
	( std::string str
	)
{
	auto idx = str.find_first_of('.');
	while(idx != std::string::npos) {
		auto replace_length = 1;
		auto next_idx = str.find_first_of('.', idx + 1);
		while(next_idx != std::string::npos && next_idx == idx + 1) {
			next_idx = str.find_first_of('.', next_idx + 1);
			replace_length += 1;
		}

		str.replace(idx, replace_length, "::");

		idx = str.find_first_of('.');
	}

	return str;
}

constexpr auto cpp_type_str
	( ecsact_builtin_type type
	)
{
	switch(type) {
		case ECSACT_I8: return "int8_t";
		case ECSACT_U8: return "uint8_t";
		case ECSACT_I16: return "int16_t";
		case ECSACT_U16: return "uint16_t";
		case ECSACT_I32: return "int32_t";
		case ECSACT_U32: return "uint32_t";
		case ECSACT_F32: return "float";
		case ECSACT_ENTITY_TYPE: return "::ecsact::entity_id";
	}
}

} // namespace ecsact::cc_lang_support
