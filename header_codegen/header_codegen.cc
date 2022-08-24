#include <vector>
#include <string>
#include "ecsact/runtime/meta.h"
#include "ecsact/codegen/write.hh"
#include "ecsact/lang-support/lang-cc.hh"

static std::vector<ecsact_component_id> get_component_ids
	( ecsact_package_id package_id
	)
{
	std::vector<ecsact_component_id> component_ids;
	component_ids.resize(ecsact_meta_count_components(package_id));
	ecsact_meta_get_component_ids(
		package_id,
		static_cast<int32_t>(component_ids.size()),
		component_ids.data(),
		nullptr
	);

	return component_ids;
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

static std::vector<ecsact_action_id> get_action_ids
	( ecsact_package_id package_id
	)
{
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

template<typename T>
static std::vector<ecsact_field_id> get_field_ids
	( T id
	)
{
	auto compo_id = ecsact_id_cast<ecsact_composite_id>(id);
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

static void write_fields
	( ecsact_composite_id  compo_id
	, std::string_view     indentation
	)
{
	using ecsact::cc_lang_support::cpp_type_str;
	using ecsact::codegen::write;
	using namespace std::string_literals;

	for(auto field_id : get_field_ids(compo_id)) {
		auto field_type = ecsact_meta_field_type(compo_id, field_id);
		auto field_name = ecsact_meta_field_name(compo_id, field_id);
		write(indentation, cpp_type_str(field_type.type), " "s, field_name);
		if(field_type.length > 1) {
			write("[", field_type.length, "]");
		}
		write(";\n");
	}
}

const char* ecsact_codegen_plugin_name() {
	return "hh";
}

void ecsact_codegen_plugin
	( ecsact_package_id package_id
	)
{
	using ecsact::cc_lang_support::cpp_identifier;
	using ecsact::codegen::write;
	using namespace std::string_literals;

	const auto namespace_str =
		cpp_identifier(ecsact_meta_package_name(package_id));

	write("namespace "s, namespace_str, " {\n\n"s);

	for(auto comp_id : get_component_ids(package_id)) {
		auto compo_id = ecsact_id_cast<ecsact_composite_id>(comp_id);
		write("struct "s, ecsact_meta_component_name(comp_id), " {\n"s);
		write_fields(compo_id, "\t"s);
		write("};\n"s);
	}

	for(auto action_id : get_action_ids(package_id)) {
		auto compo_id = ecsact_id_cast<ecsact_composite_id>(action_id);
		write("struct "s, ecsact_meta_action_name(action_id), " {\n"s);
		write("\tstruct context;\n\n"s);
		write_fields(compo_id, "\t");
		write("};\n"s);
	}

	for(auto sys_id : get_system_ids(package_id)) {
		write("struct "s, ecsact_meta_system_name(sys_id), " {\n"s);
		write("\tstruct context;\n\n"s);
		write("};\n"s);
	}

	write("\n// namespace "s, namespace_str, "\n"s);
}
