project('artemia', 'c',
  default_options: [
    'b_lto=true',
    'b_staticpic=false',
    'c_std=c2x',
    'warning_level=3',
  ],
  license: ['Apache-2.0'],
  version: '0.1.0')

c_args = [
  '-ffunction-sections',
]

link_args = [
  '-Wl,--gc-sections', '-fno-exceptions',
]


# Find libm...
cc = meson.get_compiler('c', native: false)
m_dep = cc.find_library('m', required : false)

ambiq_lib = dependency('ambiq_rba_atp')
asimple_lib = dependency('asimple_rba_atp')

# library
asimple_lib = dependency('asimple_rba_atp')

lib_sources = files([
  'src/scron.c',
  'src/artemia.c',
])

includes = include_directories([
  'include/artemia',
])

lib = library(meson.project_name(),
  lib_sources,
  include_directories: includes,
  c_args: c_args,
  link_args: link_args,
  install: true,
)

install_subdir('include', install_dir: '')

# Create a pkgconfig file
pkg = import('pkgconfig')
pkg.generate(lib, subdirs: ['', 'artemia'])

# Executable
sources = files([
  'src/main.c',
])

exe = executable(meson.project_name(),
  sources,
  link_with: lib,
  dependencies: [ambiq_lib, m_dep, asimple_lib],
  include_directories: 'include/artemia',
  c_args: c_args,
  link_args: link_args + ['-T' + meson.source_root() / 'linker.ld']
)

# Custom flash target, to make it easier to program the RedBoard, requires
# objcopy
objcopy = find_program('objcopy')

bin = custom_target(
  input : exe,
  output : exe.name().split('.')[0] + '.bin',
  command : [objcopy, '-O', 'binary', '@INPUT@', '@OUTPUT@', ],
  build_by_default: true
)

run_target('flash',
  command : ['python3', meson.source_root() / 'svl.py',
    get_option('tty'), '-f',  bin, '-b', '921600', '-v'],
  depends : bin,
)
