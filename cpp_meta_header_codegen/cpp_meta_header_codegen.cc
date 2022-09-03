#include <vector>
#include <string>
#include <cassert>
#include "ecsact/runtime/meta.hh"
#include "ecsact/codegen_plugin.h"
#include "ecsact/codegen_plugin.hh"
#include "ecsact/lang-support/lang-cc.hh"

constexpr auto GENERATED_FILE_DISCLAIMER = R"(// GENERATED FILE - DO NOT EDIT
)";

const char* ecsact_codegen_plugin_name() {
	return "meta.hh";
}

void ecsact_codegen_plugin
  ( ecsact_package_id          package_id
  , ecsact_codegen_write_fn_t  write_fn
  )
{
	using ecsact::cc_lang_support::cpp_identifier;
	using namespace std::string_literals;

  ecsact::codegen_plugin_context ctx{package_id, write_fn};

	ctx.write(GENERATED_FILE_DISCLAIMER);
	ctx.write("#pragma once\n\n");

	ctx.write("#include <cstdint>\n");
	ctx.write("#include <compare>\n");
	ctx.write("#include \"ecsact/runtime/common.h\"\n");
	ctx.write("\n");

	auto pkg_name = ecsact::meta::package_name(ctx.package_id);
	const auto namespace_str = cpp_identifier(pkg_name);

	ctx.write("namespace "s, namespace_str, " {\n\n"s);

	ctx.write("struct package {\n");

	ctx.begin_indent();
	ctx.write("static constexpr auto name() {\n");
	ctx.write("\treturn \"", pkg_name, "\";\n");
	ctx.write("}\n");

	ctx.write("using dependencies = ::ecsact::mp_list<");

	for(auto comp_id : ecsact::meta::get_component_ids(ctx.package_id)) {
		// ctx.write("\tstruct ", ecsact::meta::component_name(comp_id))
	}

	ctx.write(">;\n");
	
	ctx.end_indent();
	ctx.write("\n};\n");

	ctx.write("\n}// namespace "s, namespace_str, "\n"s);
}
