#include <vector>
#include <string>
#include <cassert>
#include <filesystem>
#include <set>
#include "ecsact/runtime/meta.hh"
#include "ecsact/codegen/plugin.h"
#include "ecsact/codegen/plugin.hh"
#include "ecsact/lang-support/lang-cc.hh"

namespace fs = std::filesystem;

constexpr auto GENERATED_FILE_DISCLAIMER = R"(// GENERATED FILE - DO NOT EDIT
)";

static void write_red_herring_static_assert(
	ecsact::codegen_plugin_context& ctx,
	std::string_view                indentation,
	std::string_view                error_message
) {
	ctx.write(indentation, "// local type to make static assert always fail\n");
	ctx.write(indentation, "struct red_herring {};\n");
	ctx.write(indentation, "static_assert(std::is_same_v<T, red_herring>, R\"(");
	ctx.write(error_message);
	ctx.write(")\");\n");
}

static void word_wrap(std::string& str, int max_length) {
	int index = 0;
	int count = 0;
	int last_word_end = 0;

	for(auto itr = str.begin(); itr != str.end(); ++itr) {
		if(count >= max_length) {
			str[last_word_end] = '\n';
			count = 0;
		} else if(*itr == '\n') {
			count = 0;
		} else {
			count += 1;
		}

		if(std::isspace(*itr)) {
			last_word_end = index;
		}

		index += 1;
	}
}

static void insert_prefix(std::string& str, std::string_view prefix) {
	for(auto itr = str.begin(); itr != str.end(); ++itr) {
		if(*itr == '\n' && itr != str.begin()) {
			itr = str.insert(std::next(itr), prefix.begin(), prefix.end());
		}
	}
}

static void write_context_method_error_body(
	ecsact::codegen_plugin_context&           ctx,
	std::string_view                          indentation,
	std::string_view                          err_msg,
	const std::set<ecsact_component_like_id>& allowed_components
) {
	const std::string msg_start = " | ";

	std::string full_err_msg =
		"\n\n[Ecsact C++ Error]: System Execution Context Misuse\n\n" +
		std::string(err_msg);
	full_err_msg += " The following components are allowed:\n";

	word_wrap(full_err_msg, 80 - static_cast<int>(msg_start.size()));

	for(auto comp_like_id : allowed_components) {
		full_err_msg += "\t- " + ecsact::meta::decl_full_name(comp_like_id) + "\n";
	}

	insert_prefix(full_err_msg, msg_start);

	write_red_herring_static_assert(ctx, indentation, full_err_msg);
}

static void write_context_get_decl(
	ecsact::codegen_plugin_context&           ctx,
	std::string_view                          indentation,
	std::string_view                          sys_like_full_name,
	const std::set<ecsact_component_like_id>& gettable_components
) {
	using namespace std::string_literals;
	ctx.write(indentation, "template<typename T>\n");
	ctx.write(indentation, "T get() {\n");

	write_context_method_error_body(
		ctx,
		std::string(indentation) + "\t",
		std::string(sys_like_full_name) +
			" context.get<T> may only be called with a component readable by the "
			"system. Did you forget to add readonly or readwrite capabilities?",
		gettable_components
	);
	ctx.write(indentation, "}\n");
}

static void write_context_update_decl(
	ecsact::codegen_plugin_context&           ctx,
	std::string_view                          indentation,
	std::string_view                          sys_like_full_name,
	const std::set<ecsact_component_like_id>& updatable_components
) {
	ctx.write(indentation, "template<typename T>\n");
	ctx.write(indentation, "void update(const T& updated_component) {\n");

	write_context_method_error_body(
		ctx,
		std::string(indentation) + "\t",
		std::string(sys_like_full_name) +
			" context.update<T> may only be called with a component writable by the "
			"system. Did you forget to add readwrite capabilities?",
		updatable_components
	);
	ctx.write(indentation, "}\n");
}

static void write_context_add_decl(
	ecsact::codegen_plugin_context&           ctx,
	std::string_view                          indentation,
	std::string_view                          sys_like_full_name,
	const std::set<ecsact_component_like_id>& addable_components
) {
	ctx.write(indentation, "template<typename T>\n");
	ctx.write(indentation, "\trequires(!std::is_empty_v<T>)\n");
	ctx.write(indentation, "void add(const T& new_component) {\n");

	write_context_method_error_body(
		ctx,
		std::string(indentation) + "\t",
		std::string(sys_like_full_name) +
			" context.add<T> may only be called with a component addable by the "
			"system. Did you forget to add adds capabilities?",
		addable_components
	);
	ctx.write(indentation, "}\n");

	ctx.write(indentation, "template<typename T>\n");
	ctx.write(indentation, "void add() {\n");
	write_context_method_error_body(
		ctx,
		std::string(indentation) + "\t",
		std::string(sys_like_full_name) +
			" context.add<T> may only be called with a component addable by the "
			"system. Did you forget to add adds capabilities?",
		addable_components
	);
	ctx.write(indentation, "}\n");
}

static void write_context_remove_decl(
	ecsact::codegen_plugin_context&           ctx,
	std::string_view                          indentation,
	std::string_view                          sys_like_full_name,
	const std::set<ecsact_component_like_id>& removable_components
) {
	ctx.write(indentation, "template<typename T>\n");
	ctx.write(indentation, "void remove() {\n");

	write_context_method_error_body(
		ctx,
		std::string(indentation) + "\t",
		std::string(sys_like_full_name) +
			" context.remove<T> may only be called with a component removable by the "
			"system. Did you forget to add removes capabilities?",
		removable_components
	);
	ctx.write(indentation, "}\n");
}

static void write_context_has_decl(
	ecsact::codegen_plugin_context& ctx,
	std::string_view                indentation
) {
	ctx.write(indentation, "template<typename T>\n");
	ctx.write(indentation, "bool has();\n");
}

static void write_context_action(
	ecsact::codegen_plugin_context& ctx,
	ecsact_action_id                act_id,
	std::string_view                indentation
) {
	using ecsact::cc_lang_support::cpp_identifier;

	std::string full_name = ecsact::meta::decl_full_name(act_id);
	std::string cpp_full_name = cpp_identifier(full_name);

	ctx.write(indentation, cpp_full_name, " action() const {\n");
	ctx.write(indentation, "\treturn _ctx.action<", cpp_full_name, ">();\n");
	ctx.write(indentation, "}\n");
}

template<typename SystemLikeID>
static void write_context_entity(
	ecsact::codegen_plugin_context& ctx,
	SystemLikeID                    id,
	std::string_view                indentation
) {
	using ecsact::cc_lang_support::cpp_identifier;

	std::string full_name = ecsact::meta::decl_full_name(id);
	std::string cpp_full_name = cpp_identifier(full_name);

	ctx.write(indentation, "ecsact_entity_id entity() const {\n");
	ctx.write(indentation, "\treturn _ctx.entity();\n");
	ctx.write(indentation, "}\n");
}

static void write_context_get_specialize(
	ecsact::codegen_plugin_context& ctx,
	ecsact_component_like_id        comp_id,
	std::string_view                indentation
) {
	using ecsact::cc_lang_support::cpp_identifier;

	auto        decl_id = ecsact_id_cast<ecsact_decl_id>(comp_id);
	std::string full_name = ecsact_meta_decl_full_name(decl_id);
	std::string cpp_full_name = cpp_identifier(full_name);

	ctx.write(indentation, "template<> ", cpp_full_name);
	ctx.write(" get<", cpp_full_name, ">() {\n");
	ctx.write(indentation, "\treturn _ctx.get<", cpp_full_name, ">();\n");
	ctx.write(indentation, "}\n");
}

static void write_context_add_specialize(
	ecsact::codegen_plugin_context& ctx,
	ecsact_component_like_id        comp_id,
	std::string_view                indentation
) {
	using ecsact::cc_lang_support::cpp_identifier;

	auto decl_id = ecsact_id_cast<ecsact_decl_id>(comp_id);

	std::string full_name = ecsact_meta_decl_full_name(decl_id);
	std::string cpp_full_name = cpp_identifier(full_name);
	auto        field_count =
		ecsact_meta_count_fields(ecsact_id_cast<ecsact_composite_id>(comp_id));

	ctx.write(indentation, "template<> void add<", cpp_full_name, ">(");
	if(field_count > 0) {
		ctx.write("const ", cpp_full_name, "& new_component");
	}
	ctx.write(") {\n");
	ctx.write(indentation, "\t_ctx.add<", cpp_full_name, ">(");
	if(field_count > 0) {
		ctx.write("new_component");
	}
	ctx.write(");\n");
	ctx.write(indentation, "}\n");
}

static void write_context_update_specialize(
	ecsact::codegen_plugin_context& ctx,
	ecsact_component_like_id        comp_id,
	std::string_view                indentation
) {
	using ecsact::cc_lang_support::cpp_identifier;

	auto decl_id = ecsact_id_cast<ecsact_decl_id>(comp_id);

	std::string full_name = ecsact_meta_decl_full_name(decl_id);
	std::string cpp_full_name = cpp_identifier(full_name);
	auto        field_count =
		ecsact_meta_count_fields(ecsact_id_cast<ecsact_composite_id>(comp_id));

	ctx.write(indentation, "template<> void update<", cpp_full_name, ">(");
	if(field_count > 0) {
		ctx.write("const ", cpp_full_name, "& updated_component");
	}
	ctx.write(") {\n");
	ctx.write(indentation, "\t_ctx.update<", cpp_full_name, ">(");
	if(field_count > 0) {
		ctx.write("updated_component");
	}
	ctx.write(");\n");
	ctx.write(indentation, "}\n");
}

static void write_context_remove_specialize(
	ecsact::codegen_plugin_context& ctx,
	ecsact_component_like_id        comp_id,
	std::string_view                indentation
) {
	using ecsact::cc_lang_support::cpp_identifier;

	auto        decl_id = ecsact_id_cast<ecsact_decl_id>(comp_id);
	std::string full_name = ecsact_meta_decl_full_name(decl_id);
	std::string cpp_full_name = cpp_identifier(full_name);

	ctx.write(indentation, "template<> void remove<", cpp_full_name, ">() {\n");
	ctx.write(indentation, "\treturn _ctx.remove<", cpp_full_name, ">();\n");
	ctx.write(indentation, "}\n");
}

void ecsact_codegen_plugin(
	ecsact_package_id         package_id,
	ecsact_codegen_write_fn_t write_fn
) {
	using ecsact::cc_lang_support::anonymous_system_name;
	using ecsact::cc_lang_support::c_identifier;
	using ecsact::cc_lang_support::cpp_identifier;
	using ecsact::meta::get_action_ids;
	using ecsact::meta::get_system_generates_ids;
	using ecsact::meta::get_system_ids;

	ecsact::codegen_plugin_context ctx{package_id, write_fn};

	ctx.write(GENERATED_FILE_DISCLAIMER);
	ctx.write("#pragma once\n\n");

	fs::path package_hh_path = ecsact_meta_package_file_path(package_id);
	fs::path pacakge_systems_h_path = package_hh_path;
	package_hh_path.replace_extension(
		package_hh_path.extension().string() + ".hh"
	);
	pacakge_systems_h_path.replace_extension(
		pacakge_systems_h_path.extension().string() + ".systems.h"
	);

	ctx.write("#include <type_traits>\n");
	ctx.write("#include \"ecsact/cpp/execution_context.hh\"\n");
	ctx.write("#include \"", package_hh_path.filename().string(), "\"\n");
	ctx.write("#include \"", pacakge_systems_h_path.filename().string(), "\"\n");

	for(auto dep_pkg_id : ecsact::meta::get_dependencies(package_id)) {
		fs::path dep_pkg_systems_hh_path =
			ecsact_meta_package_file_path(dep_pkg_id);
		dep_pkg_systems_hh_path.replace_extension(
			dep_pkg_systems_hh_path.extension().string() + ".systems.hh"
		);

		if(dep_pkg_systems_hh_path.has_parent_path()) {
			dep_pkg_systems_hh_path =
				fs::relative(dep_pkg_systems_hh_path, package_hh_path.parent_path());
		} else {
			dep_pkg_systems_hh_path = dep_pkg_systems_hh_path.filename();
		}

		ctx.write("#include \"", dep_pkg_systems_hh_path.generic_string(), "\"\n");
	}

	ctx.write("\n");

	ctx.write("\nstruct ecsact_system_execution_context;\n");

	auto write_sys_context = [&]<typename ID>(ID id, auto&& extra_body_fn) {
		constexpr bool is_action = std::is_same_v<ecsact_action_id, ID>;
		auto           sys_like_id = ecsact_id_cast<ecsact_system_like_id>(id);
		std::string    full_name =
			ecsact_meta_decl_full_name(ecsact_id_cast<ecsact_decl_id>(id));

		if constexpr(!is_action) {
			if(full_name.empty()) {
				full_name += ecsact::meta::package_name(ctx.package_id) + ".";
				full_name += anonymous_system_name(id);
			}
		} else {
			assert(!full_name.empty());
		}

		using other_contexts_t = std::unordered_map<
			ecsact_component_like_id,
			std::unordered_map<
				ecsact_field_id,
				std::unordered_map<ecsact_component_like_id, ecsact_system_capability>>>;
		other_contexts_t other_contexts;

		for(const auto& entry : ecsact::meta::system_capabilities(sys_like_id)) {
			auto comp_id = entry.first;
			auto associations =
				ecsact::meta::system_association_fields(sys_like_id, comp_id);
			for(auto field_id : associations) {
				auto assoc_caps = ecsact::meta::system_association_capabilities(
					sys_like_id,
					comp_id,
					field_id
				);
				for(auto&& [assoc_comp_id, assoc_cap] : assoc_caps) {
					other_contexts[comp_id][field_id][assoc_comp_id] = assoc_cap;
				}
			}
		}

		ctx.write("\nstruct ", cpp_identifier(full_name), "::context {\n");
		if(!other_contexts.empty()) {
			ctx.write("\ttemplate<typename T>\n");
			ctx.write("\tstruct other_contexts;\n\n");
		}
		ctx.write("\t[[no_unique_address]]\n");
		ctx.write("\t::ecsact::execution_context _ctx;\n");

		std::optional<ecsact_system_like_id> parent_sys_like_id;
		if constexpr(!is_action) {
			parent_sys_like_id = ecsact::meta::get_parent_system_id(id);
		}
		auto cap_count = ecsact_meta_system_capabilities_count(sys_like_id);

		std::vector<ecsact_component_like_id> cap_comp_ids;
		std::vector<ecsact_system_capability> caps;
		cap_comp_ids.resize(cap_count);
		caps.resize(cap_count);

		ecsact_meta_system_capabilities(
			sys_like_id,
			cap_count,
			cap_comp_ids.data(),
			caps.data(),
			nullptr
		);

		std::set<ecsact_component_like_id> add_components;
		std::set<ecsact_component_like_id> get_components;
		std::set<ecsact_component_like_id> update_components;
		std::set<ecsact_component_like_id> remove_components;
		std::set<ecsact_component_like_id> optional_components;
		auto gen_ids = get_system_generates_ids(sys_like_id);

		for(int i = 0; cap_count > i; ++i) {
			auto& comp_id = cap_comp_ids[i];
			auto& cap = caps[i];

			if((cap & ECSACT_SYS_CAP_READONLY) == ECSACT_SYS_CAP_READONLY) {
				get_components.emplace(comp_id);
			}

			if((cap & ECSACT_SYS_CAP_WRITEONLY) == ECSACT_SYS_CAP_WRITEONLY) {
				update_components.emplace(comp_id);
			}

			if((cap & ECSACT_SYS_CAP_ADDS) == ECSACT_SYS_CAP_ADDS) {
				add_components.emplace(comp_id);
			}

			if((cap & ECSACT_SYS_CAP_REMOVES) == ECSACT_SYS_CAP_REMOVES) {
				remove_components.emplace(comp_id);
			}

			if((cap & ECSACT_SYS_CAP_OPTIONAL) == ECSACT_SYS_CAP_OPTIONAL) {
				optional_components.emplace(comp_id);
			}
		}

		if(!get_components.empty()) {
			write_context_get_decl(ctx, "\t", full_name, get_components);
		}
		if(!update_components.empty()) {
			write_context_update_decl(ctx, "\t", full_name, update_components);
		}
		if(!add_components.empty()) {
			write_context_add_decl(ctx, "\t", full_name, add_components);
		}
		if(!remove_components.empty()) {
			write_context_remove_decl(ctx, "\t", full_name, remove_components);
		}
		if(!optional_components.empty()) {
			write_context_has_decl(ctx, "\t");
		}

		if(parent_sys_like_id) {
			auto parent_full_name = ecsact::meta::decl_full_name(*parent_sys_like_id);
			if(parent_full_name.empty()) {
				parent_full_name += ecsact::meta::package_name(ctx.package_id) + ".";
				parent_full_name += anonymous_system_name(*parent_sys_like_id);
			}
			auto parent_cpp_full_name = cpp_identifier(parent_full_name);
			ctx
				.write("\tconst ", parent_cpp_full_name, "::context parent() const;\n");
		}

		ctx.write("\n\n");

		for(auto get_comp_id : get_components) {
			write_context_get_specialize(ctx, get_comp_id, "\t");
		}

		for(auto add_comp_id : add_components) {
			write_context_add_specialize(ctx, add_comp_id, "\t");
		}

		for(auto update_comp_id : update_components) {
			write_context_update_specialize(ctx, update_comp_id, "\t");
		}

		for(auto remove_comp_id : remove_components) {
			write_context_remove_specialize(ctx, remove_comp_id, "\t");
		}

		write_context_entity(ctx, sys_like_id, "\t");

		extra_body_fn();

		ctx.write("};\n");
	};

	for(auto sys_id : get_system_ids(ctx.package_id)) {
		write_sys_context(sys_id, [] {});
	}

	for(auto act_id : get_action_ids(ctx.package_id)) {
		write_sys_context(act_id, [&] { write_context_action(ctx, act_id, "\t"); });
	}
}
