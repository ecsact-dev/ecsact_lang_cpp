#include <string>
#include <filesystem>
#include "ecsact/runtime/meta.hh"
#include "ecsact/codegen_plugin.h"
#include "ecsact/codegen_plugin.hh"
#include "ecsact/lang-support/lang-cc.hh"

namespace fs = std::filesystem;

constexpr auto GENERATED_FILE_DISCLAIMER = R"(// GENERATED FILE - DO NOT EDIT
)";

const char* ecsact_codegen_plugin_name() {
	return "systems.cc";
}

void ecsact_codegen_plugin(
	ecsact_package_id         package_id,
	ecsact_codegen_write_fn_t write_fn
) {
	using ecsact::cc_lang_support::c_identifier;
	using ecsact::cc_lang_support::cpp_identifier;
	using ecsact::meta::get_all_system_like_ids;
	ecsact::codegen_plugin_context ctx{package_id, write_fn};

	ctx.write(GENERATED_FILE_DISCLAIMER);

	fs::path package_systems_hh_path = ecsact_meta_package_file_path(package_id);
	package_systems_hh_path.replace_extension(
		package_systems_hh_path.extension().string() + ".systems.hh"
	);

	ctx.write("#include \"", package_systems_hh_path.filename().string(), "\"\n");

	for(auto sys_like_id : get_all_system_like_ids(ctx.package_id)) {
		std::string full_name =
			ecsact_meta_decl_full_name(ecsact_id_cast<ecsact_decl_id>(sys_like_id));

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
