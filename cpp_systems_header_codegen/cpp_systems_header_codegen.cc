#include <vector>
#include <string>
#include <cassert>
#include <filesystem>
#include <set>
#include "ecsact/runtime/meta.hh"
#include "ecsact/codegen/plugin.h"
#include "ecsact/codegen/plugin.hh"
#include "ecsact/lang-support/lang-cc.hh"
#include "ecsact/cpp_codegen_plugin_util.hh"

namespace fs = std::filesystem;
using namespace ecsact::cpp_codegen_plugin_util;

using ecsact::cc_lang_support::anonymous_system_name;
using ecsact::cc_lang_support::c_identifier;
using ecsact::cc_lang_support::cpp_identifier;
using ecsact::meta::get_action_ids;
using ecsact::meta::get_system_generates_ids;
using ecsact::meta::get_system_ids;

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

static auto write_context_other_decl(
	ecsact::codegen_plugin_context&     ctx,
	std::vector<ecsact_system_assoc_id> assoc_ids,
	std::string_view                    indentation
) -> void {
	ctx.write(
		indentation,
		"template<std::size_t Index",
		assoc_ids.size() == 1 ? " = 0" : "",
		">\n"
	);
	ctx.write(indentation, "auto other() -> other_context<Index>;\n");

	for(auto i = 0; assoc_ids.size() > i; ++i) {
		ctx.write(
			indentation,
			"template<> auto other<",
			i,
			">() -> other_context<",
			i,
			">;\n"
		);
	}
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

static void write_context_entity(ecsact::codegen_plugin_context& ctx) {
	block(ctx, "auto entity() const -> ecsact_entity_id", [&] {
		ctx.write("return _ctx.entity();\n");
	});
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

static auto write_context_other_specialize(
	ecsact::codegen_plugin_context& ctx,
	size_t                          index,
	ecsact_system_assoc_id          assoc_id
) -> void {
	ctx.write("template<> ");
	auto printer = //
		method_printer{ctx, std::format("other<{}>", index)} //
			.return_type(std::format("other_context<{}>", index));
}

struct context_body_details {
	std::set<ecsact_component_like_id> add_components;
	std::set<ecsact_component_like_id> get_components;
	std::set<ecsact_component_like_id> update_components;
	std::set<ecsact_component_like_id> remove_components;
	std::set<ecsact_component_like_id> optional_components;

	static auto from_caps(auto caps) -> context_body_details {
		auto details = context_body_details{};

		for(auto&& [comp_id, cap] : caps) {
			if((cap & ECSACT_SYS_CAP_READONLY) == ECSACT_SYS_CAP_READONLY) {
				details.get_components.emplace(comp_id);
			}

			if((cap & ECSACT_SYS_CAP_WRITEONLY) == ECSACT_SYS_CAP_WRITEONLY) {
				details.update_components.emplace(comp_id);
			}

			if((cap & ECSACT_SYS_CAP_ADDS) == ECSACT_SYS_CAP_ADDS) {
				details.add_components.emplace(comp_id);
			}

			if((cap & ECSACT_SYS_CAP_REMOVES) == ECSACT_SYS_CAP_REMOVES) {
				details.remove_components.emplace(comp_id);
			}

			if((cap & ECSACT_SYS_CAP_OPTIONAL) == ECSACT_SYS_CAP_OPTIONAL) {
				details.optional_components.emplace(comp_id);
			}
		}

		return details;
	}

	static auto from_assoc_id( //
		ecsact_system_like_id  sys_like_id,
		ecsact_system_assoc_id assoc_id
	) -> context_body_details {
		auto caps = ecsact::meta::system_assoc_capabilities(sys_like_id, assoc_id);
		auto details = from_caps(caps);
		return details;
	}

	static auto from_sys_like( //
		ecsact_system_like_id sys_like_id
	) -> context_body_details {
		auto caps = ecsact::meta::system_capabilities(sys_like_id);
		auto details = from_caps(caps);
		return details;
	}
};

/**
 * Write execution context body methods that are common between systems,
 * actions, and other (entity association) contexts.
 */
static auto write_context_body_common(
	ecsact::codegen_plugin_context& ctx,
	std::string                     ctx_name,
	context_body_details            details
) -> void {
	ctx.write("[[no_unique_address]]\n");
	ctx.write("::ecsact::execution_context _ctx;\n");

	if(!details.get_components.empty()) {
		write_context_get_decl(ctx, "\t", ctx_name, details.get_components);
	}
	if(!details.update_components.empty()) {
		write_context_update_decl(ctx, "\t", ctx_name, details.update_components);
	}
	if(!details.add_components.empty()) {
		write_context_add_decl(ctx, "\t", ctx_name, details.add_components);
	}
	if(!details.remove_components.empty()) {
		write_context_remove_decl(ctx, "\t", ctx_name, details.remove_components);
	}
	if(!details.optional_components.empty()) {
		write_context_has_decl(ctx, "\t");
	}

	for(auto get_comp_id : details.get_components) {
		write_context_get_specialize(ctx, get_comp_id, "\t");
	}

	for(auto add_comp_id : details.add_components) {
		write_context_add_specialize(ctx, add_comp_id, "\t");
	}

	for(auto update_comp_id : details.update_components) {
		write_context_update_specialize(ctx, update_comp_id, "\t");
	}

	for(auto remove_comp_id : details.remove_components) {
		write_context_remove_specialize(ctx, remove_comp_id, "\t");
	}

	write_context_entity(ctx);
}

template<typename ID>
static auto write_sys_context(
	ecsact::codegen_plugin_context& ctx,
	ID                              id,
	auto&&                          extra_body_fn
) {
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

	auto assoc_ids = ecsact::meta::system_assoc_ids(sys_like_id);

	ctx.write("\nstruct ", cpp_identifier(full_name), "::context {\n");
	if(!assoc_ids.empty()) {
		ctx.write("\ttemplate<std::size_t Index>\n");
		ctx.write("\tstruct other_context;\n");
	}

	for(auto i = 0; assoc_ids.size() > i; ++i) {
		ctx.write("\n");
		block(ctx, std::format("template<> struct other_context<{}>", i), [&] {
			write_context_body_common(
				ctx,
				std::format("{} (assoc {})", full_name, i),
				context_body_details::from_assoc_id(sys_like_id, assoc_ids[i])
			);
		});
	}

	std::optional<ecsact_system_like_id> parent_sys_like_id;
	if constexpr(!is_action) {
		parent_sys_like_id = ecsact::meta::get_parent_system_id(id);
	}

	write_context_body_common(
		ctx,
		full_name,
		context_body_details::from_sys_like(sys_like_id)
	);

	auto gen_ids = get_system_generates_ids(sys_like_id);

	if(!assoc_ids.empty()) {
		write_context_other_decl(ctx, assoc_ids, "\t");
	}

	if(parent_sys_like_id) {
		auto parent_full_name = ecsact::meta::decl_full_name(*parent_sys_like_id);
		if(parent_full_name.empty()) {
			parent_full_name += ecsact::meta::package_name(ctx.package_id) + ".";
			parent_full_name += anonymous_system_name(*parent_sys_like_id);
		}
		auto parent_cpp_full_name = cpp_identifier(parent_full_name);
		ctx.write("\tconst ", parent_cpp_full_name, "::context parent() const;\n");
	}

	ctx.write("\n\n");

	for(auto i = 0; assoc_ids.size() > i; ++i) {
		// ctx.indentation++;
		// ctx.write("\n");
		// write_context_other_specialize(ctx, i, assoc_ids[i]);
		// ctx.indentation--;
		// ctx.write("\n");
	}

	extra_body_fn();

	ctx.write("};\n");
};

void ecsact_codegen_plugin(
	ecsact_package_id         package_id,
	ecsact_codegen_write_fn_t write_fn
) {
	ecsact::codegen_plugin_context ctx{package_id, write_fn};

	ctx.write(GENERATED_FILE_DISCLAIMER);
	ctx.write("#pragma once\n\n");

	fs::path package_hh_path = ecsact_meta_package_file_path(package_id);
	fs::path package_systems_h_path = package_hh_path;
	package_hh_path.replace_extension(
		package_hh_path.extension().string() + ".hh"
	);
	package_systems_h_path.replace_extension(
		package_systems_h_path.extension().string() + ".systems.h"
	);

	ctx.write("#include <type_traits>\n");
	ctx.write("#include \"ecsact/cpp/execution_context.hh\"\n");
	ctx.write("#include \"", package_hh_path.filename().string(), "\"\n");
	ctx.write("#include \"", package_systems_h_path.filename().string(), "\"\n");

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

	for(auto sys_id : get_system_ids(ctx.package_id)) {
		write_sys_context(ctx, sys_id, [] {});
	}

	for(auto act_id : get_action_ids(ctx.package_id)) {
		write_sys_context(ctx, act_id, [&] {
			write_context_action(ctx, act_id, "\t");
		});
	}
}
