#include <vector>
#include <string>
#include <cassert>
#include <cstring>
#include <algorithm>
#include "ecsact/runtime/meta.h"
#include "ecsact/codegen_plugin.h"
#include "ecsact/codegen_plugin.hh"
#include "ecsact/lang-support/lang-cc.hh"

constexpr auto GENERATED_FILE_DISCLAIMER = R"(// GENERATED FILE - DO NOT EDIT
)";

static std::vector<ecsact_system_id> get_system_ids( //
	ecsact_package_id package_id
) {
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

static std::vector<ecsact_action_id> get_action_ids( //
	ecsact_package_id package_id
) {
	std::vector<ecsact_action_id> action_ids;
	action_ids.resize(ecsact_meta_count_actions(package_id));
	ecsact_meta_get_action_ids(
		package_id,
		static_cast<int32_t>(action_ids.size()),
		action_ids.data(),
		nullptr
	);

	return action_ids;
}

static std::string make_package_inc_guard_str(ecsact_package_id package_id) {
	using ecsact::cc_lang_support::c_identifier;

	auto package_name = ecsact_meta_package_name(package_id);
	auto inc_guard_str = c_identifier(package_name);
	std::transform(
		inc_guard_str.begin(),
		inc_guard_str.end(),
		inc_guard_str.begin(),
		std::toupper
	);

	inc_guard_str += "_H";

	return inc_guard_str;
}

template<typename T>
static void write_system_impl_fn_decl(
	ecsact::codegen_plugin_context& ctx,
	T                               id
) {
	using ecsact::cc_lang_support::c_identifier;

	auto        decl_id = ecsact_id_cast<ecsact_decl_id>(id);
	std::string full_name = ecsact_meta_decl_full_name(decl_id);
	if(full_name.empty()) {
		return;
	}

	ctx.write(
		"ECSACT_SYSTEM_EXTERN void ",
		c_identifier(full_name),
		"(struct ecsact_system_execution_context*);\n"
	);
}

const char* ecsact_codegen_plugin_name() {
	return "systems.h";
}

void ecsact_codegen_plugin(
	ecsact_package_id         package_id,
	ecsact_codegen_write_fn_t write_fn
) {
	using namespace std::string_literals;

	ecsact::codegen_plugin_context ctx{package_id, write_fn};
	const auto inc_guard_str = make_package_inc_guard_str(package_id);

	ctx.write(GENERATED_FILE_DISCLAIMER);
	ctx.write("#ifndef ", inc_guard_str, "\n");
	ctx.write("#define ", inc_guard_str, "\n\n");

	ctx.write("#ifdef __cplusplus\n");
	ctx.write("#	define ECSACT_SYSTEM_EXTERN extern \"C\"\n");
	ctx.write("#else\n");
	ctx.write("#	define ECSACT_SYSTEM_EXTERN extern\n");
	ctx.write("#endif\n");
	ctx.write("\n");

	for(auto action_id : get_action_ids(ctx.package_id)) {
		write_system_impl_fn_decl(ctx, action_id);
	}

	for(auto sys_id : get_system_ids(ctx.package_id)) {
		write_system_impl_fn_decl(ctx, sys_id);
	}

	ctx.write("\n");
	ctx.write("#undef ECSACT_SYSTEM_EXTERN\n");
	ctx.write("#endif//", inc_guard_str, "\n");
}
