#pragma once

#include <type_traits>
#include "ecsact/lib.hh"

namespace ecsact {
	template<typename SystemLikeT>
	struct is_system_like : std::bool_constant<false> {};
	template<typename SystemLikeT>
	using is_system_like_v = typename is_system_like<SystemLikeT>::value;

	template<typename SystemT>
	struct is_system : std::bool_constant<false> {};
	template<typename SystemT>
	using is_system_v = typename is_system<SystemT>::value;

	template<typename ActionT>
	struct is_action : std::bool_constant<false> {};
	template<typename ActionT>
	using is_action_v = typename is_action<ActionT>::value;

	template<typename SystemLikeT>
	struct system_capabilities_info {
		using readonly_components = mp_list<>;
		using readwrite_components = mp_list<>;
		using writeonly_components = mp_list<>;
		using optional_components = mp_list<>;
		using adds_components = mp_list<>;
		using removes_components = mp_list<>;
		using include_components = mp_list<>;
		using exclude_components = mp_list<>;
		using generates = mp_list<>;
		using associations = mp_list<>;
	};
}
