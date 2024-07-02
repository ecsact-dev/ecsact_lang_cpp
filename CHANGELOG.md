# Changelog
All notable changes to this project will be documented in this file. See [conventional commits](https://www.conventionalcommits.org/) for commit guidelines.

- - -
## 0.4.5 - 2024-07-02
#### Bug Fixes
- add missing return type in context.remove (#200) - (02f96d9) - Ezekiel Warren
- add missing return type in context.add (#199) - (551f476) - Ezekiel Warren
#### Features
- add constexpr for components with assoc fields (#204) - (c067502) - Ezekiel Warren
#### Miscellaneous Chores
- **(deps)** update dependency ecsact_runtime to v0.6.6 (#201) - (e76f35f) - renovate[bot]
- update codegen plugin interface (#207) - (92f7c73) - Ezekiel Warren
- sync with ecsact_common (#203) - (ac1290a) - seaubot
- sync with ecsact_common (#202) - (381c353) - seaubot

- - -

## 0.4.4 - 2024-06-19
#### Features
- support new assoc api in C++ codegen (#196) - (a1e518f) - Ezekiel Warren
- remove unused meta header codegen plugin - (8d70238) - Ezekiel Warren
#### Miscellaneous Chores
- **(deps)** update dependency ecsact_cli to v0.3.11 (#146) - (bba404f) - renovate[bot]
- remove libarchive git_override and update cli (#198) - (d8e504f) - Austin Kelway
- sync with ecsact_common (#187) - (89cec22) - seaubot

- - -

## 0.4.3 - 2024-05-27
#### Features
- add tail to block generation util (#195) - (0084f28) - Ezekiel Warren

- - -

## 0.4.2 - 2024-05-23
#### Features
- generate indexed fields in plugins (#194) - (edb5c0d) - Ezekiel Warren

- - -

## 0.4.1 - 2024-04-08
#### Bug Fixes
- don't use expand template on binary (#189) - (4e77dbf) - Ezekiel Warren

- - -

## 0.4.0 - 2024-04-04
#### Miscellaneous Chores
- **(deps)** update dependency bazel to v7.1.1 (#183) - (425b8e5) - renovate[bot]
- sync with ecsact_common (#185) - (48afe87) - seaubot
- dev use ecsact_cli + update deps (#186) - (55d0b95) - Ezekiel Warren

- - -

## 0.3.4 - 2024-04-01
#### Bug Fixes
- mark ecsact bazel toolchain as dev dep - (283f682) - Ezekiel Warren

- - -

## 0.3.3 - 2024-03-27
#### Miscellaneous Chores
- use c++20 and add llvm toolchain for linux ci tests (#184) - (f457ba1) - Austin Kelway

- - -

## 0.3.2 - 2024-03-13
#### Bug Fixes
- codegen plugin target/path improvements (#182) - (f357d7f) - Ezekiel Warren
#### Miscellaneous Chores
- **(deps)** update hedron_compile_commands digest to 4d56714 (#180) - (af45104) - renovate[bot]
- **(deps)** update hedron_compile_commands digest to ceeb5db (#178) - (7857a46) - renovate[bot]
- **(deps)** update hedron_compile_commands digest to 3700f76 (#177) - (338051b) - renovate[bot]
- **(deps)** update hedron_compile_commands digest to f41ec09 (#176) - (1907657) - renovate[bot]
- **(deps)** update hedron_compile_commands digest to 5f9f8ba (#175) - (ff39b31) - renovate[bot]
- **(deps)** update hedron_compile_commands digest to b998dca (#174) - (ff1a89e) - renovate[bot]
- **(deps)** update hedron_compile_commands digest to 388cc00 (#173) - (77114c7) - renovate[bot]
- **(deps)** update hedron_compile_commands digest to 46ffd1f (#172) - (7b35a93) - renovate[bot]
- **(deps)** update hedron_compile_commands digest to 9d438af (#171) - (183bc5d) - renovate[bot]
- **(deps)** update hedron_compile_commands digest to eac41ee (#170) - (20654cb) - renovate[bot]
- **(deps)** update hedron_compile_commands digest to af16787 (#169) - (0badaa0) - renovate[bot]
- **(deps)** update hedron_compile_commands digest to 3b1745c (#168) - (7a58ee2) - renovate[bot]
- **(deps)** update hedron_compile_commands digest to 4a04d55 (#167) - (a2cf4e1) - renovate[bot]
- **(deps)** update com_grail_bazel_toolchain digest to 214fc1c (#166) - (edda593) - renovate[bot]
- **(deps)** update hedron_compile_commands digest to eca42c6 (#164) - (a1947d9) - renovate[bot]
- **(deps)** update com_grail_bazel_toolchain digest to ade23e0 (#163) - (65a0e84) - renovate[bot]
- **(deps)** update com_grail_bazel_toolchain digest to 7500623 (#160) - (9970d35) - renovate[bot]
- **(deps)** update com_grail_bazel_toolchain digest to 1e5f3c6 (#158) - (43d700f) - renovate[bot]
- **(deps)** update hedron_compile_commands digest to ac6411f (#157) - (2446782) - renovate[bot]
- **(deps)** update com_grail_bazel_toolchain digest to 0a9feb7 (#156) - (38fbfc5) - renovate[bot]
- **(deps)** update com_grail_bazel_toolchain digest to 42fa12b (#155) - (8421efa) - renovate[bot]
- **(deps)** update com_grail_bazel_toolchain digest to a76d197 (#154) - (f4cdafe) - renovate[bot]
- **(deps)** update com_grail_bazel_toolchain digest to 91abcad (#153) - (ea60469) - renovate[bot]
- **(deps)** update com_grail_bazel_toolchain digest to 2733561 (#150) - (1b7175e) - renovate[bot]
- sync with ecsact_common (#152) - (25fb752) - seaubot

- - -

## 0.3.1 - 2023-09-21
#### Miscellaneous Chores
- **(deps)** update bazel c++ tooling repositories (#144) - (a14819c) - renovate[bot]
- sync with ecsact_common (#149) - (75d8a01) - seaubot
- sync with ecsact_common (#147) - (b7b4fc2) - seaubot

- - -

## 0.3.0 - 2023-09-19
#### Bug Fixes
- **(meta)** transient fields_info (#141) - (b0497e1) - Ezekiel Warren
#### Features
- bzlmodify (#148) - (369a6f2) - Ezekiel Warren
- parallel execution available in meta codegen (#142) - (4ac52fc) - Ezekiel Warren
- added decl_name function to meta header codegen (#136) - (519a9f5) - Ezekiel Warren
- handy block codegen plugin util function - (74ebca8) - Ezekiel Warren
- added codegen plugin utility functions (#134) - (54df803) - Ezekiel Warren
#### Miscellaneous Chores
- **(deps)** update bazel c++ tooling repositories (#143) - (d098c2d) - renovate[bot]
- **(deps)** update com_grail_bazel_toolchain digest to ceeedcc (#140) - (4e9d547) - renovate[bot]
- **(deps)** update com_grail_bazel_toolchain digest to b397ad2 (#139) - (87acaee) - renovate[bot]
- **(deps)** update com_grail_bazel_toolchain digest to 41ff2a0 (#138) - (4d12e7d) - renovate[bot]
- **(deps)** update hedron_compile_commands digest to 3dddf20 (#137) - (637b027) - renovate[bot]
- **(deps)** update hedron_compile_commands digest to 80ac7ef (#135) - (9af3ce1) - renovate[bot]

- - -

## 0.2.0 - 2023-05-01
#### Features
- lazy execution values in meta codegen (#133) - (63f129c) - Ezekiel Warren
#### Miscellaneous Chores
- add cog.toml - (46fac95) - Ezekiel Warren
- using ecsact_codegen and ecsact_cli (#132) - (e2ce8de) - Ezekiel Warren

- - -

Changelog generated by [cocogitto](https://github.com/cocogitto/cocogitto).
