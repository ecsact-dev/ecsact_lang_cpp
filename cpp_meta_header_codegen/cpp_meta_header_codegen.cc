#include <vector>
#include <string>
#include <cassert>
#include <algorithm>
#include <ranges>
#include <tuple>
#include "ecsact/runtime/meta.hh"
#include "ecsact/codegen_plugin.h"
#include "ecsact/codegen_plugin.hh"
#include "ecsact/lang-support/lang-cc.hh"

namespace fs = std::filesystem;
namespace stdr = std::ranges;

constexpr auto GENERATED_FILE_DISCLAIMER = R"(// GENERATED FILE - DO NOT EDIT
)";

template<typename SystemLikeID>
static std::string sys_full_name
	( ecsact_package_id  package_id
	, SystemLikeID       id
	)
{
	using ecsact::cc_lang_support::anonymous_system_name;

	auto full_name = ecsact::meta::decl_full_name(id);
	if(full_name.empty()) {
		full_name = ecsact::meta::package_name(package_id) + ".";
		full_name += anonymous_system_name(id);
	}

	return full_name;
}

template<typename SystemLikeID>
static void write_system_execution_order
	( ecsact::codegen_plugin_context&  ctx
	, SystemLikeID                     sys_id
	)
{
	using ecsact::cc_lang_support::cpp_identifier;

	auto full_name = sys_full_name(ctx.package_id, sys_id);

	ctx.write("::ecsact::mp_list<");
	ctx.write(cpp_identifier(full_name), ", ");
	ctx.write("::ecsact::mp_list<");
	ctx.write_each(
		", ",
		ecsact::meta::get_child_system_ids(sys_id),
		[&](ecsact_system_id child_sys_id) {
			write_system_execution_order(ctx, child_sys_id);
		}
	);
	ctx.write(">>");
}

const char* ecsact_codegen_plugin_name() {
	return "meta.hh";
}

void ecsact_codegen_plugin
  ( ecsact_package_id          package_id
  , ecsact_codegen_write_fn_t  write_fn
  )
{
	using ecsact::cc_lang_support::anonymous_system_name;
	using ecsact::cc_lang_support::cpp_identifier;
	using namespace std::string_literals;

  ecsact::codegen_plugin_context ctx{package_id, write_fn};

	ctx.write(GENERATED_FILE_DISCLAIMER);
	ctx.write("#pragma once\n\n");

	fs::path package_hh_path = ecsact_meta_package_file_path(package_id);
	package_hh_path.replace_extension(
		package_hh_path.extension().string() + ".hh"
	);

	ctx.write("#include <cstdint>\n");
	ctx.write("#include <compare>\n");
	ctx.write("#include \"ecsact/runtime/common.h\"\n");
	ctx.write("#include \"ecsact/lib.hh\"\n");
	ctx.write("#include \"", package_hh_path.filename().string(), "\"\n");
	ctx.write("\n");

	auto pkg_name = ecsact::meta::package_name(ctx.package_id);
	const auto namespace_str = cpp_identifier(pkg_name);

	ctx.write("namespace "s, namespace_str, " {\n\n"s);

	++ctx.indentation;
	ctx.write("struct package {\n");
	ctx.write("static constexpr auto name() {\n");
	ctx.write("\treturn \"", pkg_name, "\";\n");
	ctx.write("}\n");

	ctx.write("using dependencies = ::ecsact::mp_list<");
	ctx.write_each(
		", ",
		ecsact::meta::get_dependencies(ctx.package_id),
		[&](ecsact_package_id dep_pkg_id) {
			ctx.write(
				"::",
				cpp_identifier(ecsact::meta::package_name(dep_pkg_id)),
				"::package"
			);
		}
	);
	ctx.write(">;\n");

	ctx.write("using components = ::ecsact::mp_list<");
	ctx.write_each(
		", ",
		ecsact::meta::get_component_ids(ctx.package_id),
		[&](ecsact_component_id comp_id) {
			ctx.write(cpp_identifier(ecsact::meta::decl_full_name(comp_id)));
		}
	);
	ctx.write(">;\n");

	ctx.write("using systems = ::ecsact::mp_list<");
	ctx.write_each(
		", ",
		ecsact::meta::get_system_ids(ctx.package_id),
		[&](ecsact_system_id sys_id) {
			auto name = ecsact::meta::decl_full_name(sys_id);
			if(name.empty()) {
				name = pkg_name + "." + anonymous_system_name(sys_id);
			}
			ctx.write(cpp_identifier(name));
		}
	);
	ctx.write(">;\n");

	ctx.write("using actions = ::ecsact::mp_list<");
	ctx.write_each(
		", ",
		ecsact::meta::get_action_ids(ctx.package_id),
		[&](ecsact_action_id comp_id) {
			ctx.write(cpp_identifier(ecsact::meta::decl_full_name(comp_id)));
		}
	);
	ctx.write(">;\n");

	ctx.write(
		"/** List of pairs. First is system. Second is list of child systems. */\n"
	);
	ctx.write("using execution_order = ::ecsact::mp_list<\n\t");

	ctx.write_each(
		",\n\t",
		ecsact::meta::get_top_level_systems(ctx.package_id),
		[&](ecsact_system_like_id sys_like_id) {
			write_system_execution_order(ctx, sys_like_id);
		}
	);

	ctx.write("\n>;\n");
	
	--ctx.indentation;
	ctx.write("\n};\n");

	ctx.write("\n}// namespace "s, namespace_str, "\n"s);
}
