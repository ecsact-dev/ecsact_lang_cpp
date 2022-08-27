#include <vector>
#include <string>
#include <cassert>
#include <filesystem>
#include <set>
#include "ecsact/runtime/meta.h"
#include "ecsact/codegen_plugin.h"
#include "ecsact/codegen_plugin.hh"
#include "ecsact/lang-support/lang-cc.hh"

namespace fs = std::filesystem;

constexpr auto GENERATED_FILE_DISCLAIMER = R"(// GENERATED FILE - DO NOT EDIT
)";

static bool has_parent_system(ecsact_system_id id) {
	auto parent_id = ecsact_meta_get_parent_system_id(id);
	return parent_id != (ecsact_system_like_id)-1;
}

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

static void write_context_add_specialize
	( ecsact::codegen_plugin_context&  ctx
	, ecsact_component_id              comp_id
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
		ctx.write("const ", cpp_full_name, "& updated_component");
	}
	ctx.write(") {\n");
	ctx.write(indentation, "\t_ctx.add<", cpp_full_name, ">(");
	if(field_count > 0) {
		ctx.write("updated_component");
	}
	ctx.write(");\n");
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
			continue;
		}

		ctx.write("\nstruct ", cpp_identifier(full_name), "::context {\n");
		ctx.write("\t[[no_unique_address]]\n");
		ctx.write("\t::ecsact::execution_context _ctx;\n");

		auto sys_like_id = ecsact_id_cast<ecsact_system_like_id>(sys_id);
		auto cap_count = ecsact_meta_system_capabilities_count(sys_like_id);

		std::vector<ecsact_component_id> cap_comp_ids;
		std::vector<ecsact_system_capability> caps;
		cap_comp_ids.reserve(cap_count);
		caps.reserve(cap_count);

		ecsact_meta_system_capabilities(
			sys_like_id,
			cap_count,
			cap_comp_ids.data(),
			caps.data(),
			nullptr
		);

		std::set<ecsact_component_id> add_components;
		std::set<ecsact_component_id> get_components;
		std::set<ecsact_component_id> update_components;
		std::set<ecsact_component_id> remove_components;

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
		}

		if(!get_components.empty()) write_context_get_decl(ctx, "\t");
		if(!update_components.empty()) write_context_update_decl(ctx, "\t");
		if(!add_components.empty()) write_context_add_decl(ctx, "\t");
		if(!remove_components.empty()) write_context_remove_decl(ctx, "\t");

		for(auto add_comp_id : add_components) {
			write_context_add_specialize(ctx, add_comp_id, "\t");
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
