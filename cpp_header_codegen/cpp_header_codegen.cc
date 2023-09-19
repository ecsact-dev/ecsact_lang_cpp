#include <vector>
#include <string>
#include <cassert>
#include "ecsact/runtime/meta.hh"
#include "ecsact/codegen/plugin.h"
#include "ecsact/codegen/plugin.hh"
#include "ecsact/lang-support/lang-cc.hh"

constexpr auto GENERATED_FILE_DISCLAIMER = R"(// GENERATED FILE - DO NOT EDIT
)";

template<typename T>
static void write_constexpr_id(
	ecsact::codegen_plugin_context& ctx,
	const char*                     id_type_name,
	T                               id,
	std::string_view                indentation
) {
	ctx.write(
		indentation,
		"static constexpr auto id = static_cast<",
		id_type_name,
		">(",
		static_cast<int32_t>(id),
		");\n"
	);
}

template<typename T>
static std::vector<ecsact_field_id> get_field_ids(T id) {
	ecsact_composite_id compo_id;
	if constexpr(std::is_same_v<ecsact_composite_id, T>) {
		compo_id = id;
	} else {
		compo_id = ecsact_id_cast<ecsact_composite_id>(id);
	}
	std::vector<ecsact_field_id> field_ids;
	field_ids.resize(ecsact_meta_count_fields(compo_id));
	ecsact_meta_get_field_ids(
		compo_id,
		static_cast<int32_t>(field_ids.size()),
		field_ids.data(),
		nullptr
	);

	return field_ids;
}

static bool has_parent_system(ecsact_system_id id) {
	auto parent_id = ecsact_meta_get_parent_system_id(id);
	return parent_id != (ecsact_system_like_id)-1;
}

static void write_fields(
	ecsact::codegen_plugin_context& ctx,
	ecsact_composite_id             compo_id,
	std::string_view                indentation
) {
	using ecsact::cc_lang_support::cpp_identifier;
	using ecsact::cc_lang_support::cpp_type_str;
	using namespace std::string_literals;

	auto full_name =
		ecsact_meta_decl_full_name(ecsact_id_cast<ecsact_decl_id>(compo_id));

	for(auto field_id : get_field_ids(compo_id)) {
		auto field_type = ecsact_meta_field_type(compo_id, field_id);
		auto field_name = ecsact_meta_field_name(compo_id, field_id);

		ctx.write(indentation);
		switch(field_type.kind) {
			case ECSACT_TYPE_KIND_BUILTIN:
				ctx.write(cpp_type_str(field_type.type.builtin));
				break;
			case ECSACT_TYPE_KIND_ENUM:
				ctx.write(ecsact_meta_enum_name(field_type.type.enum_id));
				break;
		}
		ctx.write(" "s, field_name);

		if(field_type.length > 1) {
			ctx.write("[", field_type.length, "]");
		}
		ctx.write(";\n");
	}

	ctx.write(
		indentation,
		"auto operator<=>(const ",
		cpp_identifier(full_name),
		"&) const = default;\n"
	);
}

static void write_system_impl_decl(
	ecsact::codegen_plugin_context& ctx,
	std::string_view                indentation
) {
	ctx.write(indentation, "struct context;\n");
	ctx.write(indentation, "static void impl(context&);\n");
}

static void write_system_struct(
	ecsact::codegen_plugin_context& ctx,
	ecsact_system_id                sys_id,
	std::string                     indentation
) {
	using namespace std::string_literals;
	using ecsact::cc_lang_support::anonymous_system_name;
	using ecsact::meta::get_child_system_ids;

	std::string sys_name = ecsact_meta_system_name(sys_id);
	if(!sys_name.empty()) {
		ctx.write(indentation, "struct "s, sys_name, " {\n"s);
		write_constexpr_id(ctx, "ecsact_system_id", sys_id, indentation + "\t");
		for(auto child_system_id : get_child_system_ids(sys_id)) {
			write_system_struct(ctx, child_system_id, indentation + "\t");
		}
		write_system_impl_decl(ctx, indentation + "\t");
		ctx.write(indentation, "};\n"s);
	} else {
		ctx.write(indentation, "struct "s, anonymous_system_name(sys_id), " {\n");
		write_constexpr_id(ctx, "ecsact_system_id", sys_id, indentation + "\t");
		ctx.write(indentation, "\tstruct context;\n");
		ctx.write(indentation, "};\n"s);

		for(auto child_system_id : get_child_system_ids(sys_id)) {
			write_system_struct(ctx, child_system_id, indentation);
		}
	}
}

void ecsact_codegen_plugin(
	ecsact_package_id         package_id,
	ecsact_codegen_write_fn_t write_fn
) {
	using ecsact::cc_lang_support::cpp_identifier;
	using namespace std::string_literals;
	using ecsact::meta::get_action_ids;
	using ecsact::meta::get_child_system_ids;
	using ecsact::meta::get_component_ids;
	using ecsact::meta::get_enum_ids;
	using ecsact::meta::get_enum_values;
	using ecsact::meta::get_system_ids;
	using ecsact::meta::get_transient_ids;

	ecsact::codegen_plugin_context ctx{package_id, write_fn};

	ctx.write(GENERATED_FILE_DISCLAIMER);
	ctx.write("#pragma once\n\n");

	ctx.write("#include <cstdint>\n");
	ctx.write("#include <compare>\n");
	ctx.write("#include \"ecsact/runtime/common.h\"\n");
	ctx.write("\n");

	const auto namespace_str =
		cpp_identifier(ecsact_meta_package_name(ctx.package_id));

	ctx.write("namespace "s, namespace_str, " {\n\n"s);

	for(auto enum_id : get_enum_ids(ctx.package_id)) {
		ctx.write("enum class "s, ecsact_meta_enum_name(enum_id), "{");
		++ctx.indentation;
		ctx.write("\n");

		for(auto& enum_value : get_enum_values(enum_id)) {
			ctx.write(enum_value.name, " = ", enum_value.value, ",\n");
		}
		ctx.write("};");
		--ctx.indentation;
	}

	for(auto comp_id : get_component_ids(ctx.package_id)) {
		auto compo_id = ecsact_id_cast<ecsact_composite_id>(comp_id);
		ctx.write("struct "s, ecsact_meta_component_name(comp_id), " {\n"s);
		ctx.write("\tstatic constexpr bool transient = false;\n");
		write_constexpr_id(ctx, "ecsact_component_id", comp_id, "\t");
		write_fields(ctx, compo_id, "\t"s);
		ctx.write("};\n"s);
	}

	for(auto comp_id : get_transient_ids(ctx.package_id)) {
		auto compo_id = ecsact_id_cast<ecsact_composite_id>(comp_id);
		ctx.write("struct "s, ecsact_meta_transient_name(comp_id), " {\n"s);
		ctx.write("\tstatic constexpr bool transient = true;\n");
		write_constexpr_id(ctx, "ecsact_transient_id", comp_id, "\t");
		write_fields(ctx, compo_id, "\t"s);
		ctx.write("};\n"s);
	}

	for(auto action_id : get_action_ids(ctx.package_id)) {
		auto compo_id = ecsact_id_cast<ecsact_composite_id>(action_id);
		ctx.write("struct "s, ecsact_meta_action_name(action_id), " {\n"s);
		write_constexpr_id(ctx, "ecsact_action_id", compo_id, "\t");
		for(auto child_system_id : get_child_system_ids(action_id)) {
			write_system_struct(ctx, child_system_id, "\t");
		}
		write_system_impl_decl(ctx, "\t");
		write_fields(ctx, compo_id, "\t");
		ctx.write("};\n"s);
	}

	for(auto sys_id : get_system_ids(ctx.package_id)) {
		if(has_parent_system(sys_id)) {
			continue;
		}

		write_system_struct(ctx, sys_id, "");
	}

	ctx.write("\n}// namespace "s, namespace_str, "\n"s);
}
