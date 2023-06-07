#pragma once

#include <cstdint>
#include <type_traits>
#include <array>
#include <string_view>
#include "ecsact/lib.hh"
#include "ecsact/runtime/common.h"
#include "ecsact/runtime/definitions.h"

namespace ecsact::detail {
template<typename>
constexpr bool unimplemented_by_meta_cpp_codegen = false;
}

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

template<typename SystemT>
struct system_lazy_execution_iteration_rate;

template<typename SystemT>
constexpr auto system_lazy_execution_iteration_rate_v =
	system_lazy_execution_iteration_rate<SystemT>::value;

template<typename SystemT>
struct system_parallel_execution;

template<typename SystemLikeT>
constexpr auto system_parallel_execution_v =
	system_parallel_execution<SystemLikeT>::value;

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

template<typename CompositeT>
constexpr std::size_t fields_count();

struct field_info {
	/**
	 * Offset in component layout. Same as `offsetof(Component, field)`.
	 */
	std::size_t offset;

	/**
	 * The field storage type. In the builtin type fields this is the actual
	 * field type, but in the enum case it's the underlying type (storage type.)
	 */
	ecsact_builtin_type storage_type;

	/**
	 * @see`ecsact_field_type::length`
	 */
	std::size_t length;

	template<typename FieldType>
	constexpr auto get(const void* component_data) const noexcept {
		return *reinterpret_cast<const FieldType*>(
			reinterpret_cast<const char*>(component_data) + offset
		);
	}
};

/**
 * @returns std::array of `field_info` instances
 */
template<typename C>
constexpr std::array<field_info, fields_count<C>()> fields_info();

/**
 * @returns the ecsact declaration full name. The same value that should be
 *          returned by ecsact_meta_decl_name
 */
template<typename Decl>
constexpr auto decl_full_name() -> std::string_view {
	static_assert(detail::unimplemented_by_meta_cpp_codegen<Decl>, R"(
 -----------------------------------------------------------------------------
| (!) CODEGEN ERROR                                                           |
| `ecsact::decl_full_name<>` template specialization cannot be found. This is |
| typically generated by ecsact_cpp_meta_header_codegen. Make sure the passed |
| in template parameter (Decl) is an ecsact declaration type and the .meta.hh |
| file is included in the source this method is being used.                   |
 -----------------------------------------------------------------------------
)");
	return {};
}

} // namespace ecsact
