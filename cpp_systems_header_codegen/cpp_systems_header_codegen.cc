#include <vector>
#include <string>
#include <cassert>
#include <filesystem>
#include <set>
#include "ecsact/runtime/meta.hh"
#include "ecsact/codegen_plugin.h"
#include "ecsact/codegen_plugin.hh"
#include "ecsact/lang-support/lang-cc.hh"

namespace fs = std::filesystem;

constexpr auto GENERATED_FILE_DISCLAIMER = R"(// GENERATED FILE - DO NOT EDIT
)";

static std::vector<ecsact_system_id> get_system_ids
	( ecsact_package_id package_id
	)
{
	std::vector<ecsact_system_id> system_ids;
	system_ids.resize(ecsact_meta_count_systems(package_id));
	ecsact_meta_get_system_ids(
		package_id,
		static_cast<int32_t>(system_ids.size()),
		system_ids.data(),
		nullptr
	);

	return system_ids;
}

static void write_context_get_decl
	( ecsact::codegen_plugin_context&  ctx
	, std::string_view                 indentation
	)
{	
	ctx.write(indentation, "template<typename T>\n");
	ctx.write(indentation, "T get();\n");
}

static void write_context_update_decl
	( ecsact::codegen_plugin_context&  ctx
	, std::string_view                 indentation
	)
{
	ctx.write(indentation, "template<typename T>\n");
	ctx.write(indentation, "void update(const T& updated_component);\n");
}

static void write_context_add_decl
	( ecsact::codegen_plugin_context&  ctx
	, std::string_view                 indentation
	)
{
	ctx.write(indentation, "template<typename T>\n");
	ctx.write(indentation, "\trequires(!std::is_empty_v<T>)\n");
	ctx.write(indentation, "void add(const T& new_component);\n");
	ctx.write(indentation, "template<typename T>\n");
	ctx.write(indentation, "void add();\n");
}

static void write_context_remove_decl
	( ecsact::codegen_plugin_context&  ctx
	, std::string_view                 indentation
	)
{
	ctx.write(indentation, "template<typename T>\n");
	ctx.write(indentation, "void remove();\n");
}

static void write_context_has_decl
	( ecsact::codegen_plugin_context&  ctx
	, std::string_view                 indentation
	)
{
	ctx.write(indentation, "template<typename T>\n");
	ctx.write(indentation, "bool has();\n");
}

template<typename SystemLikeID>
static void write_context_generate_defn
	( ecsact::codegen_plugin_context&  ctx
	, SystemLikeID                     sys_id
	, ecsact_system_generates_id       gen_id
	, std::string_view                 indentation
	)
{
	using ecsact::meta::get_system_generates_components;
	using ecsact::cc_lang_support::cpp_identifier;
	using ecsact::meta::decl_full_name;

	ctx.write(indentation, "inline void generate(");

	ctx.write_each(
		", ",
		get_system_generates_components(sys_id, gen_id),
		[&](const auto& entry) {
			ecsact_component_id comp_id = entry.first;
			ecsact_system_generate flag = entry.second;
			auto comp_full_name = decl_full_name(comp_id);
			auto cpp_comp_full_name = cpp_identifier(comp_full_name);

			if(flag == ECSACT_SYS_GEN_OPTIONAL) {
				ctx.write(cpp_comp_full_name);
			} else {
				ctx.write(cpp_comp_full_name);
			}
		}
	);

	ctx.write(") {}\n");
}

static void write_context_get_specialize
	( ecsact::codegen_plugin_context&  ctx
	, ecsact_component_like_id         comp_id
	, std::string_view                 indentation
	)
{
	using ecsact::cc_lang_support::cpp_identifier;

	auto decl_id = ecsact_id_cast<ecsact_decl_id>(comp_id);
	std::string full_name = ecsact_meta_decl_full_name(decl_id);
	std::string cpp_full_name = cpp_identifier(full_name);

	ctx.write(indentation, "template<> ", cpp_full_name);
	ctx.write(" get<", cpp_full_name, ">() {\n");
	ctx.write(indentation, "\treturn _ctx.get<", cpp_full_name, ">();\n");
	ctx.write(indentation, "}\n");
}

static void write_context_add_specialize
	( ecsact::codegen_plugin_context&  ctx
	, ecsact_component_like_id         comp_id
	, std::string_view                 indentation
	)
{
	using ecsact::cc_lang_support::cpp_identifier;

	auto decl_id = ecsact_id_cast<ecsact_decl_id>(comp_id);

	std::string full_name = ecsact_meta_decl_full_name(decl_id);
	std::string cpp_full_name = cpp_identifier(full_name);
	auto field_count = ecsact_meta_count_fields(
		ecsact_id_cast<ecsact_composite_id>(comp_id)
	);

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

static void write_context_update_specialize
	( ecsact::codegen_plugin_context&  ctx
	, ecsact_component_like_id         comp_id
	, std::string_view                 indentation
	)
{
	using ecsact::cc_lang_support::cpp_identifier;

	auto decl_id = ecsact_id_cast<ecsact_decl_id>(comp_id);

	std::string full_name = ecsact_meta_decl_full_name(decl_id);
	std::string cpp_full_name = cpp_identifier(full_name);
	auto field_count = ecsact_meta_count_fields(
		ecsact_id_cast<ecsact_composite_id>(comp_id)
	);

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

static void write_context_remove_specialize
	( ecsact::codegen_plugin_context&  ctx
	, ecsact_component_like_id         comp_id
	, std::string_view                 indentation
	)
{
	using ecsact::cc_lang_support::cpp_identifier;

	auto decl_id = ecsact_id_cast<ecsact_decl_id>(comp_id);
	std::string full_name = ecsact_meta_decl_full_name(decl_id);
	std::string cpp_full_name = cpp_identifier(full_name);

	ctx.write(indentation, "template<> void remove<", cpp_full_name, ">() {\n");
	ctx.write(indentation, "\treturn _ctx.remove<", cpp_full_name, ">();\n");
	ctx.write(indentation, "}\n");
}

const char* ecsact_codegen_plugin_name() {
	return "systems.hh";
}

void ecsact_codegen_plugin
  ( ecsact_package_id          package_id
  , ecsact_codegen_write_fn_t  write_fn
  )
{
	using ecsact::cc_lang_support::cpp_identifier;
	using ecsact::cc_lang_support::c_identifier;
	using ecsact::cc_lang_support::anonymous_system_name;
	using ecsact::meta::get_system_generates_ids;

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

	ctx.write("\nstruct ecsact_system_execution_context;\n");

	auto system_ids = get_system_ids(ctx.package_id);

	for(auto sys_id : system_ids) {
		std::string full_name = ecsact_meta_decl_full_name(
			ecsact_id_cast<ecsact_decl_id>(sys_id)
		);

		if(full_name.empty()) {
			full_name += ecsact::meta::package_name(ctx.package_id) + ".";
			full_name += anonymous_system_name(sys_id);
		}

		ctx.write("\nstruct ", cpp_identifier(full_name), "::context {\n");
		ctx.write("\t[[no_unique_address]]\n");
		ctx.write("\t::ecsact::execution_context _ctx;\n");

		auto sys_like_id = ecsact_id_cast<ecsact_system_like_id>(sys_id);
		auto parent_sys_like_id = ecsact::meta::get_parent_system_id(sys_id);
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

		for(int i=0; cap_count > i; ++i) {
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

		if(!get_components.empty()) write_context_get_decl(ctx, "\t");
		if(!update_components.empty()) write_context_update_decl(ctx, "\t");
		if(!add_components.empty()) write_context_add_decl(ctx, "\t");
		if(!remove_components.empty()) write_context_remove_decl(ctx, "\t");
		if(!optional_components.empty()) write_context_has_decl(ctx, "\t");

		if(parent_sys_like_id) {
			auto parent_full_name = ecsact::meta::decl_full_name(*parent_sys_like_id);
			if(parent_full_name.empty()) {
				parent_full_name += ecsact::meta::package_name(ctx.package_id) + ".";
				parent_full_name += anonymous_system_name(*parent_sys_like_id);
			}
			auto parent_cpp_full_name = cpp_identifier(parent_full_name);
			ctx.write(
				"\tconst ", parent_cpp_full_name, "::context parent() const;\n"
			);
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

		for(auto gen_id : gen_ids) {
			write_context_generate_defn(ctx, sys_id, gen_id, "\t");
		}

		ctx.write("};\n");
	}

	for(auto sys_id : system_ids) {
		std::string full_name = ecsact_meta_decl_full_name(
			ecsact_id_cast<ecsact_decl_id>(sys_id)
		);

		if(full_name.empty()) {
			continue;
		}

		auto cpp_full_name = cpp_identifier(full_name);

		ctx.write(
			"void ",
			c_identifier(full_name),
			"(struct ecsact_system_execution_context* cctx) {\n"
		);

		ctx.write("\t", cpp_full_name, "::context ctx{cctx};\n");
		ctx.write("\t", cpp_full_name, "::impl(ctx);\n");

		ctx.write("}\n");
	}
}
