project('sbrt', 'c', 'cpp', 
  default_options : ['c_std=c11', 'cpp_std=c++20'], 
  version: '1.0.0'
)

fmt_dep = dependency('fmt')

sources = [
  'src/common.cpp',
  'src/main.cpp',
  'src/pass/isel_ir_dag_check_pass.cpp'
]

include_dirs = [
  'include',
  'deps/expected/',
  'deps/stacktrace/',
  'deps/magic_enum/include/magic_enum/',
  '.'
]

conf_data = configuration_data()
conf_data.set('VERSION', meson.project_version())
conf_data.set('MESON_C_COMPILER', meson.get_compiler('c').get_id())
conf_data.set('MESON_CXX_COMPILER', meson.get_compiler('cpp').get_id())

configure_file(input: 'build_config.h.in', output: 'build_config.h', configuration : conf_data)

client = executable('sbrt', sources, 
  dependencies: [fmt_dep],
  cpp_pch: 'pch/pch.h', 
  cpp_args : ['-DFMT_HEADER_ONLY', '-ftime-trace'], 
  link_args: ['-lbfd'],
  include_directories: include_directories(include_dirs),
)

