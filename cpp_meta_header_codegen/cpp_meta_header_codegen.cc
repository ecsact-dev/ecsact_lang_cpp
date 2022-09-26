#include <vector>
#include <string>
#include <cassert>
#include <algorithm>
#include <ranges>
#include <tuple>
#include <map>
#include "ecsact/runtime/meta.hh"
#include "ecsact/codegen_plugin.h"
#include "ecsact/codegen_plugin.hh"
#include "ecsact/lang-support/lang-cc.hh"

namespace fs = std::filesystem;
namespace stdr = std::ranges;

constexpr auto GENERATED_FILE_DISCLAIMER = R"(// GENERATED FILE - DO NOT EDIT
)";

template<typename SystemLikeID>
static std::string get_sys_full_name
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

	auto full_name = get_sys_full_name(ctx.package_id, sys_id);

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


static void for_each_applicable_comps_map(auto& maps, auto cap, auto&& cb) {
	if((cap & ECSACT_SYS_CAP_READWRITE) == ECSACT_SYS_CAP_READWRITE) {
		cb(maps.readwrite_comps);
	} else
	if((cap & ECSACT_SYS_CAP_READONLY) == ECSACT_SYS_CAP_READONLY) {
		cb(maps.readonly_comps);
	} else
	if((cap & ECSACT_SYS_CAP_WRITEONLY) == ECSACT_SYS_CAP_WRITEONLY) {
		cb(maps.writeonly_comps);
	}

	if((cap & ECSACT_SYS_CAP_OPTIONAL) == ECSACT_SYS_CAP_OPTIONAL) {
		cb(maps.optional_comps);
	}

	if((cap & ECSACT_SYS_CAP_REMOVES) == ECSACT_SYS_CAP_REMOVES) {
		cb(maps.removes_comps);
	}

	if((cap & ECSACT_SYS_CAP_ADDS) == ECSACT_SYS_CAP_ADDS) {
		cb(maps.adds_comps);
	}

	if((cap & ECSACT_SYS_CAP_INCLUDE) == ECSACT_SYS_CAP_INCLUDE) {
		cb(maps.include_comps);
	}

	if((cap & ECSACT_SYS_CAP_EXCLUDE) == ECSACT_SYS_CAP_EXCLUDE) {
		cb(maps.exclude_comps);
	}
}

struct system_comp_maps {
	using system_comps_map_t = std::map
		< ecsact_system_like_id
		, std::vector<ecsact_component_like_id>
		>;
	using sys_assoc_comp_maps_t = std::map<
		ecsact_system_like_id,
		std::map<
			ecsact_component_like_id,
			std::map<ecsact_field_id, std::vector<ecsact_component_like_id>>
		>
	>;
	system_comps_map_t readwrite_comps;
	system_comps_map_t readonly_comps;
	system_comps_map_t writeonly_comps;
	system_comps_map_t optional_comps;
	system_comps_map_t adds_comps;
	system_comps_map_t removes_comps;
	system_comps_map_t include_comps;
	system_comps_map_t exclude_comps;

	sys_assoc_comp_maps_t readwrite_assoc_comps;
	sys_assoc_comp_maps_t readonly_assoc_comps;
	sys_assoc_comp_maps_t writeonly_assoc_comps;
	sys_assoc_comp_maps_t optional_assoc_comps;
	sys_assoc_comp_maps_t adds_assoc_comps;
	sys_assoc_comp_maps_t removes_assoc_comps;
	sys_assoc_comp_maps_t include_assoc_comps;
	sys_assoc_comp_maps_t exclude_assoc_comps;

	template<typename SystemLikeID>
	void set_comp_maps(SystemLikeID id) {
		const auto sys_like_id = ecsact_id_cast<ecsact_system_like_id>(id);
		for(auto& entry : ecsact::meta::system_capabilities(sys_like_id)) {
			auto comp = entry.first;
			auto cap = entry.second;

			for_each_applicable_comps_map(*this, cap, [&](system_comps_map_t& map) {
				map[sys_like_id].push_back(comp);
			});

			auto assoc_fields = ecsact::meta::system_association_fields(
				sys_like_id,
				comp
			);

			for(auto& field_id : assoc_fields) {
				auto other_caps = ecsact::meta::system_association_capabilities(
					sys_like_id,
					comp,
					field_id
				);

				for(auto& other_entry : other_caps) {
					auto other_comp = other_entry.first;
					auto other_cap = other_entry.second;
					// TODO
				}
			}
		}

		for(auto child : ecsact::meta::get_child_system_ids(id)) {
			set_comp_maps(child);
		}
	}
};

static void write_system_capabilities_info_struct
	( ecsact::codegen_plugin_context&  ctx
	, auto                             id
	)
{
	using ecsact::cc_lang_support::cpp_identifier;
	using ecsact::meta::get_system_generates_components;

	auto sys_like_id = ecsact_id_cast<ecsact_system_like_id>(id);
	auto full_name = get_sys_full_name(ctx.package_id, sys_like_id);
	auto cpp_full_name = cpp_identifier(full_name);

	ctx.write("\ntemplate<>\n");
	ctx.write("struct ecsact::system_capabilities_info<", cpp_full_name, "> {");
	++ctx.indentation;
	ctx.write("\n");

	std::vector<ecsact_component_like_id> readwrite_comps;
	std::vector<ecsact_component_like_id> readonly_comps;
	std::vector<ecsact_component_like_id> writeonly_comps;
	std::vector<ecsact_component_like_id> optional_comps;
	std::vector<ecsact_component_like_id> adds_comps;
	std::vector<ecsact_component_like_id> removes_comps;
	std::vector<ecsact_component_like_id> include_comps;
	std::vector<ecsact_component_like_id> exclude_comps;

	for(auto& entry : ecsact::meta::system_capabilities(sys_like_id)) {
		auto comp = entry.first;
		auto cap = entry.second;

		if((cap & ECSACT_SYS_CAP_READWRITE) == ECSACT_SYS_CAP_READWRITE) {
			readwrite_comps.push_back(comp);
		} else
		if((cap & ECSACT_SYS_CAP_READONLY) == ECSACT_SYS_CAP_READONLY) {
			readonly_comps.push_back(comp);
		} else
		if((cap & ECSACT_SYS_CAP_WRITEONLY) == ECSACT_SYS_CAP_WRITEONLY) {
			writeonly_comps.push_back(comp);
		}

		if((cap & ECSACT_SYS_CAP_OPTIONAL) == ECSACT_SYS_CAP_OPTIONAL) {
			optional_comps.push_back(comp);
		}

		if((cap & ECSACT_SYS_CAP_REMOVES) == ECSACT_SYS_CAP_REMOVES) {
			removes_comps.push_back(comp);
		}

		if((cap & ECSACT_SYS_CAP_ADDS) == ECSACT_SYS_CAP_ADDS) {
			adds_comps.push_back(comp);
		}

		if((cap & ECSACT_SYS_CAP_INCLUDE) == ECSACT_SYS_CAP_INCLUDE) {
			include_comps.push_back(comp);
		}

		if((cap & ECSACT_SYS_CAP_EXCLUDE) == ECSACT_SYS_CAP_EXCLUDE) {
			exclude_comps.push_back(comp);
		}
	}

	auto write_comp_full_name = [&](ecsact_component_like_id comp_id) {
		auto full_name = ecsact::meta::decl_full_name(comp_id);
		auto cpp_full_name = cpp_identifier(full_name);
		ctx.write(cpp_full_name);
	};

	ctx.write("using readonly_components = mp_list<\n\t");
	ctx.write_each(",\n\t", readonly_comps, write_comp_full_name);
	ctx.write("\n>;\n");

	ctx.write("using readwrite_components = mp_list<\n\t");
	ctx.write_each(",\n\t", readwrite_comps, write_comp_full_name);
	ctx.write("\n>;\n");

	ctx.write("using writeonly_components = mp_list<\n\t");
	ctx.write_each(",\n\t", writeonly_comps, write_comp_full_name);
	ctx.write("\n>;\n");

	ctx.write("using optional_components = mp_list<\n\t");
	ctx.write_each(",\n\t", optional_comps, write_comp_full_name);
	ctx.write("\n>;\n");

	ctx.write("using adds_components = mp_list<\n\t");
	ctx.write_each(",\n\t", adds_comps, write_comp_full_name);
	ctx.write("\n>;\n");

	ctx.write("using removes_components = mp_list<\n\t");
	ctx.write_each(",\n\t", removes_comps, write_comp_full_name);
	ctx.write("\n>;\n");

	ctx.write("using include_components = mp_list<\n\t");
	ctx.write_each(",\n\t", include_comps, write_comp_full_name);
	ctx.write("\n>;\n");

	ctx.write("using exclude_components = mp_list<\n\t");
	ctx.write_each(",\n\t", exclude_comps, write_comp_full_name);
	ctx.write("\n>;\n");

	ctx.write("using generates = mp_list<\n\t");
	ctx.write_each(
		",\n\t",
		ecsact::meta::get_system_generates_ids(sys_like_id),
		[&](ecsact_system_generates_id gen_id) {
			auto generates_comps = get_system_generates_components(
				sys_like_id,
				gen_id
			);

			ctx.write("mp_list<\n\t\t");
			++ctx.indentation;
			ctx.write_each(
				",\n\t",
				generates_comps,
				[&](auto& entry) {
					auto comp_full_name = ecsact::meta::decl_full_name(entry.first);
					switch(entry.second) {
						case ECSACT_SYS_GEN_REQUIRED:
							ctx.write("entity_component_required");
							break;
						case ECSACT_SYS_GEN_OPTIONAL:
							ctx.write("entity_component_optional");
							break;
					}
					ctx.write("<", cpp_identifier(comp_full_name), ">");
				}
			);
			--ctx.indentation;
			ctx.write("\n\t>");
		}
	);

	ctx.write("\n>;\n");
	ctx.write("using associations = mp_list<>;\n");

	--ctx.indentation;
	ctx.write("\n};\n");
}

static void write_system_comps_map_using
	( ecsact::codegen_plugin_context&              ctx
	, std::string_view                             alias_name
	, const system_comp_maps::system_comps_map_t&  map
	)
{
	using ecsact::cc_lang_support::cpp_identifier;

	ctx.write("using ", alias_name, " = ::ecsact::mp_list<\n\t");
	ctx.write_each(
		",\n\t",
		map,
		[&](auto& entry) {
			auto sys_full_name = get_sys_full_name(ctx.package_id, entry.first);
			ctx.write("::ecsact::mp_list<");
			ctx.write(cpp_identifier(sys_full_name), ", ");
			ctx.write("::ecsact::mp_list<");
			ctx.write_each(
				", ",
				entry.second,
				[&](ecsact_component_like_id comp_id) {
					auto comp_full_name = ecsact::meta::decl_full_name(comp_id);
					ctx.write(cpp_identifier(comp_full_name));
				}
			);
			ctx.write(">>");
		}
	);
	ctx.write("\n>;\n");
}

static void write_system_assoc_comps_map_using
	( ecsact::codegen_plugin_context&                 ctx
	, std::string_view                                alias_name
	, const system_comp_maps::sys_assoc_comp_maps_t&  maps
	)
{
	using ecsact::cc_lang_support::cpp_identifier;

	ctx.write("using ", alias_name, " = ::ecsact::mp_list<\n\t");
	
	ctx.write("\n>;\n");
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
	using ecsact::meta::get_top_level_systems;
	using ecsact::meta::get_system_generates_components;
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
	ctx.write("#include \"ecsact/cpp/type_info.hh\"\n");
	ctx.write("#include \"ecsact/lib.hh\"\n");
	ctx.write("#include \"", package_hh_path.filename().string(), "\"\n");
	ctx.write("\n");

	auto pkg_name = ecsact::meta::package_name(ctx.package_id);
	auto top_level_systems = get_top_level_systems(ctx.package_id);
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

	ctx.write("using transients = ::ecsact::mp_list<");
	ctx.write_each(
		", ",
		ecsact::meta::get_transient_ids(ctx.package_id),
		[&](ecsact_transient_id trans_id) {
			ctx.write(cpp_identifier(ecsact::meta::decl_full_name(trans_id)));
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
		top_level_systems,
		[&](ecsact_system_like_id sys_like_id) {
			write_system_execution_order(ctx, sys_like_id);
		}
	);
	ctx.write("\n>;\n");

	ctx.write("using generates = ::ecsact::mp_list<\n\t");
	ctx.write_each(
		",\n\t",
		ecsact::meta::get_all_system_like_ids(package_id),
		[&](ecsact_system_like_id sys_like_id) {
			auto full_name = get_sys_full_name(ctx.package_id, sys_like_id);

			ctx.write("::ecsact::mp_list<");
			ctx.write(cpp_identifier(full_name), ", ");
			ctx.write("::ecsact::mp_list<");
			ctx.write_each(
				", ",
				ecsact::meta::get_system_generates_ids(sys_like_id),
				[&](ecsact_system_generates_id gen_id) {
					auto generates_comps = get_system_generates_components(
						sys_like_id,
						gen_id
					);

					ctx.write("::ecsact::mp_list<");
					ctx.write_each(
						", ",
						generates_comps,
						[&](auto& entry) {
							auto comp_full_name = ecsact::meta::decl_full_name(entry.first);
							switch(entry.second) {
								case ECSACT_SYS_GEN_REQUIRED:
									ctx.write("::ecsact::entity_component_required");
									break;
								case ECSACT_SYS_GEN_OPTIONAL:
									ctx.write("::ecsact::entity_component_optional");
									break;
							}
							ctx.write("<", cpp_identifier(comp_full_name), ">");
						}
					);
					ctx.write(">");
				}
			);
			ctx.write(">>");
		}
	);
	ctx.write("\n>;\n");
	
	--ctx.indentation;
	ctx.write("\n};\n");

	ctx.write("\n}// namespace "s, namespace_str, "\n"s);

	for(auto& sys_id : ecsact::meta::get_system_ids(ctx.package_id)) {
		write_system_capabilities_info_struct(ctx, sys_id);
	}

	for(auto& act_id : ecsact::meta::get_action_ids(ctx.package_id)) {
		write_system_capabilities_info_struct(ctx, act_id);
	}
}
