project('DrSpread', 'c', version: '1.0.0')
COMPAT_VERSION='1.0.0' # for macos dylib

# untested
if host_machine.system() == 'windows'
  ignore_bogus_deprecations = ['-D_CRT_NONSTDC_NO_WARNINGS', '-D_CRT_SECURE_NO_WARNINGS']
else
  ignore_bogus_deprecations = []
endif

if host_machine.system() == 'darwin'
  arches = ['-arch', 'x86_64', '-arch', 'arm64']
else
  arches = []
endif

cc = meson.get_compiler('c')
m_dep = cc.find_library('m', required: false)

install_headers('drspread.h', subdir:'DrSpread')

drspread_dylib = both_libraries(
  'drspread',
  'drspread.c',
  install:true,
  dependencies:[m_dep],
  version: meson.project_version(),
  soversion: meson.project_version(),
  darwin_versions:[COMPAT_VERSION, meson.project_version()],
  c_args: ignore_bogus_deprecations + arches,
  link_args: arches,
)
drspread_test_lib = both_libraries(
  'drspread-test',
  'drspread.c',
  install:true,
  dependencies:[m_dep],
  version: meson.project_version(),
  soversion: meson.project_version(),
  darwin_versions:[COMPAT_VERSION, meson.project_version()],
  c_args: ignore_bogus_deprecations + arches + '-DDRSP_INTRINS=1',
  link_args: arches,
)

executable('drspread', 'drspread_cli.c', install:false, c_args:ignore_bogus_deprecations, dependencies:[m_dep])

test_drspread_dy = executable('test-drspread-dy-link', 'TestDrSpread.c', link_with:drspread_test_lib, c_args: ignore_bogus_deprecations+arches+['-DDRSPREAD_TEST_DYLINK=1'], dependencies:[m_dep])
test('test-drspread-dy', test_drspread_dy)

test_drspread_unity = executable('test-drspread-unity', 'TestDrSpread.c', c_args: ignore_bogus_deprecations+arches, dependencies:[m_dep])
test('test-drspread-unity', test_drspread_unity)
