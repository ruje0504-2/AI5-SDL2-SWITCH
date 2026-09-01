project('libai5', 'c',
  default_options : ['c_std=c11', 'default_library=static'])
add_project_arguments('-D_DEFAULT_SOURCE', language : 'c')

static_libs = false
if host_machine.system() == 'windows'
    static_libs = true
endif

png = dependency('libpng', static : static_libs)

nulib_sources = [
  'nulib/src/buffer.c',
  'nulib/src/command.c',
  'nulib/src/file.c',
  'nulib/src/hashset.c',
  'nulib/src/port.c',
  'nulib/src/sds.c',
  'nulib/src/system.c',
  'nulib/src/utfsjis.c'
]

ai5_sources = [
  'src/a6.c',
  'src/anim.c',
  'src/arc/open.c',
  'src/ccd.c',
  'src/cg/akb.c',
  'src/cg/cg.c',
  'src/cg/gp4.c',
  'src/cg/gp8.c',
  'src/cg/gpr.c',
  'src/cg/gpx.c',
  'src/cg/g16_24_32.c',
  'src/cg/gcc.c',
  'src/cg/png.c',
  'src/game.c',
  'src/lzss.c',
  'src/mes/codes.c',
  'src/mes/parse.c',
  'src/mes/parse_aiw.c',
  'src/mes/print.c',
  'src/mes/print_aiw.c',
  'src/mes/system.c',
]

inc = include_directories('include', 'nulib/include')

libai5 = library('ai5', [nulib_sources, ai5_sources],
                 dependencies : [png],
                 include_directories : inc)

libai5_dep = declare_dependency(include_directories : inc, link_with : libai5)
