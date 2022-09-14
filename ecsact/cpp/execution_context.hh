#pragma once

#include <type_traits>
#include "ecsact/runtime/dynamic.h"

struct ecsact_system_execution_context;

namespace ecsact {

	struct execution_context {
		[[no_unique_address]]
		ecsact_system_execution_context* const _ctx;

		/**
		 * Safe way to create a readonly execution context
		 */
		static const execution_context make_readonly
			( const ecsact_system_execution_context* const ctx
			)
		{
			return execution_context{
				const_cast<ecsact_system_execution_context* const>(ctx)
			};
		}

		template<typename C>
			requires (!std::is_empty_v<C>)
		C get() const {
			C comp;
			ecsact_system_execution_context_get(
				_ctx,
				ecsact_id_cast<ecsact_component_like_id>(C::id),
				&comp
			);
			return comp;
		}

		template<typename C>
			requires (!std::is_empty_v<C>)
		void update(const C& updated_component) {
			ecsact_system_execution_context_update(
				_ctx,
				ecsact_id_cast<ecsact_component_like_id>(C::id),
				&updated_component
			);
		}

		template<typename C>
		bool has() const {
			return ecsact_system_execution_context_has(
				_ctx,
				ecsact_id_cast<ecsact_component_like_id>(C::id)
			);
		}

		template<typename C>
			requires (!std::is_empty_v<C>)
		void add(const C& new_component) {
			ecsact_system_execution_context_add(
				_ctx,
				ecsact_id_cast<ecsact_component_like_id>(C::id),
				&new_component
			);
		}

		template<typename C>
			requires (std::is_empty_v<C>)
		void add() {
			ecsact_system_execution_context_add(
				_ctx,
				ecsact_id_cast<ecsact_component_like_id>(C::id),
				nullptr
			);
		}

		template<typename C>
		void remove() {
			ecsact_system_execution_context_remove(
				_ctx,
				ecsact_id_cast<ecsact_component_like_id>(C::id)
			);
		}

		template<typename... C>
		void generate(C&&... components) {
			std::vector<ecsact_component_id> component_ids;
			std::vector<const void*> components_data;
			component_ids.reserve(sizeof...(C));
			components_data.reserve(sizeof...(C));

			(component_ids.push_back(C::id), ...);
			(components_data.push_back(&components), ...);

			ecsact_system_execution_context_generate(
				_ctx,
				sizeof...(C),
				component_ids.data(),
				components_data.data()
			);
		}

		const execution_context parent() const {
			return execution_context::make_readonly(
				ecsact_system_execution_context_parent(_ctx)
			);
		}

		bool same(const execution_context& other) const {
			return ecsact_system_execution_context_same(_ctx, other._ctx);
		}

		execution_context other(ecsact_entity_id entity) {
			return execution_context{
				ecsact_system_execution_context_other(_ctx, entity)
			};
		}

		ecsact_system_like_id id() const {
			return ecsact_system_execution_context_id(_ctx);
		}
	};

}
