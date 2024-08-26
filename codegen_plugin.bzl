load("@bazel_skylib//rules:copy_file.bzl", "copy_file")
load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_test")
load("@rules_ecsact//ecsact:defs.bzl", "ecsact_codegen_plugin")
load("@rules_ecsact//ecsact/private:ecsact_codegen_plugin.bzl", "EcsactCodegenPluginInfo")

def _cc_ecsact_codegen_plugin_impl(ctx):
    # type: (ctx) -> list

    plugin = None  # type: File | None
    files = ctx.attr.cc_binary[DefaultInfo].files.to_list()  # type: list[File]

    for file in files:
        if file.extension == "so":
            plugin = file
        if file.extension == "dll":
            plugin = file

    plugin_well_known_path = ctx.actions.declare_file(
        "{}.{}".format(ctx.attr.name, plugin.extension),
        sibling = plugin,
    )
    ctx.actions.symlink(
        target_file = plugin,
        output = plugin_well_known_path,
    )

    return [
        DefaultInfo(
            files = depset([plugin_well_known_path]),
        ),
        EcsactCodegenPluginInfo(
            output_extension = ctx.attr.output_extension,
            outputs = ctx.attr.outputs,
            plugin = plugin,
            data = [plugin],
        ),
    ]

_cc_ecsact_codegen_plugin = rule(
    implementation = _cc_ecsact_codegen_plugin_impl,
    attrs = {
        "cc_binary": attr.label(mandatory = True),
        "output_extension": attr.string(mandatory = False),
        "outputs": attr.string_list(mandatory = False)
    },
)

_generated_src = """
#include "ecsact/codegen/plugin.h"

const char* ecsact_codegen_plugin_name() {{
    return "{output_extension}";
}}
"""

def _cc_ecsact_codegen_plugin_src_impl(ctx):
    output_cc_src = ctx.actions.declare_file("{}.plugin_name.cc".format(ctx.attr.name))
    if not ctx.attr.output_extension and len(ctx.attr.outputs) != 0
        fail("You cannot use both output extension and outputs")
    if ctx.attr.output_extension!= None: 
        ctx.actions.write(
            output = output_cc_src,
            content = _generated_src.format(output_extension = ctx.attr.output_extension),
        )
    else: 
        ctx.actions.write(
            output = output_cc_src,
            content = _generated_src.format(output_extension = ctx.attr.name),
        )

    return [
        DefaultInfo(files = depset([output_cc_src])),
    ]

_cc_ecsact_codegen_plugin_src = rule(
    implementation = _cc_ecsact_codegen_plugin_src_impl,
    attrs = {
        "output_extension": attr.string(mandatory = False),
    },
)

def cc_ecsact_codegen_plugin(name = None, srcs = [], deps = [], defines = [], no_validate_test = False, output_extension = None, outputs = None, **kwargs):
    """Create ecsact codegen plugin with C++

    NOTE: ecsact_codegen_plugin_name() is automatically generated for you based
    on the `output_extension` attribute.

    Args:
        name: Passed to underling cc_binary
        srcs: Passed to underling cc_binary
        deps: Passed to underling cc_binary
        defines: Passed to underling cc_binary
        output_extension: File extension the plugin writes to. Cannot be used with outputs
        outputs: A list of well known filenames to output. Cannot be used with output_extension
        no_validate_test: Don't create plugin validation test (not recommended)
        **kwargs: Passed to underling cc_binary
    """
    name_hash = hash(name)
    cc_binary(
        name = "{}__bin".format(name_hash),
        srcs = srcs + [
            "@ecsact_runtime//dylib:dylib.cc",
            ":{}__src".format(name_hash),
        ],
        deps = deps + [
            "@ecsact_runtime//:dylib",
            "@ecsact_runtime//dylib:meta",
            "@ecsact_codegen//:plugin",
        ],
        defines = defines + ["ECSACT_META_API_LOAD_AT_RUNTIME"],
        linkshared = True,
        **kwargs
    )

    if(output_extension != None):
        _cc_ecsact_codegen_plugin_src(
            name = "{}__src".format(name_hash),
            output_extension = output_extension,
        )
    else:
        _cc_ecsact_codegen_plugin_src(
            name = "{}__src".format(name_hash),
        )

    _cc_ecsact_codegen_plugin(
        name = name,
        cc_binary = ":{}__bin".format(name_hash),
        output_extension = output_extension,
        outputs = outputs
    )
