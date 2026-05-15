project(
  'vhdsa-ref',
  ['c', 'cpp'],
  version: '2.0.3',
  meson_version: '>=0.57',
  default_options: [
    'c_std=c99',
    'cpp_std=c++17',
    'warning_level=3',
    'b_lto=true',
    'b_ndebug=if-release',
  ],
)

cc = meson.get_compiler('c')
cpp = meson.get_compiler('cpp')
conf_data = configuration_data()

# compile flags and defines
flags = []
c_flags = []
cpp_flags = []
defines = []

if cc.get_id() != 'msvc'
  c_flags += [
    '-Werror=implicit-function-declaration',
    '-Werror=vla',
    '-Werror=incompatible-pointer-types',
  ]
  flags += ['-Wshadow', '-Wno-psabi']
  defines += '-D_GNU_SOURCE'

  if get_option('march-native').enabled()
    flags += '-march=native'
  endif
else
  warnings = []
  # function inlining
  warnings += ['4710', '4711']
  # padding of structs
  warnings += ['4820', '4324']
  # .. in include paths
  warnings += '4464'
  # undefined macros evaluating to 0 in MSVC headers
  warnings += '4668'
  # Spectre mitigation
  warnings += '5045'
  # initialization of aggregate types with non-constant values is valid C99
  warnings += '4204'
  # nameless structs/unions are part of C11
  warnings += '4201'
  # conditional expression is constant
  warnings += '4127'
  # unary minus of unsigned types
  warnings += '4146'
  # 'inline' used more than once
  warnings += '4141'

  foreach w : warnings
    flags += '/wd' + w
  endforeach

  defines += '-D_CRT_SECURE_NO_WARNINGS'
endif
c_flags = cc.get_supported_arguments(flags) + cc.get_supported_arguments(c_flags)
cpp_flags = cpp.get_supported_arguments(flags) + cc.get_supported_arguments(cpp_flags)
linker_flags = cc.get_supported_link_arguments(flags)

# check compiler if compiler supports __builtin_cpu_supports
has_usable_builtin = cc.compiles(
  '''
#include "cpu.h"

#if !defined(BUILTIN_CPU_SUPPORTED) || defined(BUILTIN_CPU_SUPPORTED_BROKEN_BMI2)
#error "__builtin_cpu_supports is not supported"
#endif
''',
  name: 'compiler supports __builtin_cpu_supports',
  include_directories: include_directories('.'),
)

# check compiler if compiler supports AES-NI
aesni_flag = cc.get_supported_arguments('-maes')
supports_aesni = cc.links(
  '''
#include <wmmintrin.h>

int main() {
  _mm_aesenc_si128(_mm_setzero_si128(), _mm_setzero_si128());
}
''',
  name: 'compiler supports AES-NI',
  args: [aesni_flag],
)
# conf_data.set('HAVE_AESNI', supports_aesni)
conf_data.set('HAVE_AESNI', false) # Disabled AES-NI

# check compiler if compiler supports AVX2
avx2_flag = cc.get_supported_arguments('-mavx2')
supports_avx2 = cc.links(
  '''
#include <immintrin.h>

int main() {
  _mm256_xor_si256(_mm256_setzero_si256(), _mm256_setzero_si256());
}
''',
  name: 'compiler supports AVX2',
  args: [avx2_flag],
)
# conf_data.set('HAVE_AVX2', supports_avx2)
conf_data.set('HAVE_AVX2', false) # Disabled AVX2

sse2_flag = cc.get_supported_arguments('-msse2')
supports_loadu_si64 = cc.links(
  '''
#include <immintrin.h>
#include <stdint.h>

int main() {
  uint64_t v = 0;
  void* p = &v;
  _mm_loadu_si64(p);
}
''',
  name: 'compiler supports _mm_loadu_si64',
  args: [sse2_flag],
)
conf_data.set('HAVE_MM_LOADU_SI64', supports_loadu_si64)

# check availability of some headers
conf_data.set('HAVE_ASM_HWCAP_H', cc.has_header('asm/hwcap.h'))
conf_data.set('HAVE_SYS_RANDOM_H', cc.has_header('sys/random.h'))
conf_data.set('HAVE_SYS_AUXV_H', cc.has_header('sys/auxv.h'))

# check availability of some functions
conf_data.set(
  'HAVE_ALIGNED_ALLOC',
  cc.has_header_symbol('stdlib.h', 'aligned_alloc', args: defines),
)
conf_data.set(
  'HAVE_POSIX_MEMALIGN',
  cc.has_header_symbol('stdlib.h', 'posix_memalign', args: defines),
)
conf_data.set('HAVE_MEMALIGN', cc.has_header_symbol('malloc.h', 'memalign', args: defines))
conf_data.set(
  'HAVE_GETRANDOM',
  cc.has_header_symbol('sys/random.h', 'getrandom', args: defines),
)
conf_data.set(
  'HAVE_ARC4RANDOM_BUF',
  cc.has_header_symbol('stdlib.h', 'arc4random_buf', args: defines),
)
conf_data.set(
  'HAVE_EXPLICIT_BZERO',
  cc.has_header_symbol('string.h', 'explicit_bzero', args: defines),
)
conf_data.set(
  'HAVE_CONSTTIME_MEMEQUAL',
  cc.has_header_symbol('string.h', 'consttime_memequal', args: defines),
)
conf_data.set(
  'HAVE_TIMINGSAFE_BCMP',
  cc.has_header_symbol('string.h', 'timingsafe_bcmp', args: defines),
)
conf_data.set(
  'HAVE_PTHREAD_SETAFFINITY_NP',
  cc.has_header_symbol('pthread.h', 'pthread_setaffinity_np', args: defines),
)

# parameter sets
# After changing the parameter sets, be sure to run crypto_sign_generator.py to create the corresponding meson files.
parameter_sets = []

# parameter set: VHDSA-128s
param_128s = configuration_data()
param_128s.set('PARAM', '128S')
param_128s.set('PARAM_L', '128s')
param_128s.set('LAMBDA', 128)
param_128s.set('Nst', 4)
param_128s.set('Ske', 40)
param_128s.set('R', 10)
param_128s.set('Senc', 160)
param_128s.set('BETA', 1)
param_128s.set('ELL', 1280)
param_128s.set('Lke', 448)
param_128s.set('Lenc', 832)
param_128s.set('TAU', 11)
param_128s.set('W_GRIND', 7)
param_128s.set('T_OPEN', 120)
param_128s.set('SIG_SIZE', 4794)
param_128s.set('PK_SIZE', 32)
param_128s.set('SK_SIZE', 32)
param_128s.set('OWF_INPUT_SIZE', 16)
param_128s.set('OWF_OUTPUT_SIZE', 16)
parameter_sets += param_128s

# parameter set: VHDSA-128f
param_128f = configuration_data()
param_128f.set('PARAM', '128F')
param_128f.set('PARAM_L', '128f')
param_128f.set('LAMBDA', 128)
param_128f.set('Nst', 4)
param_128f.set('Ske', 40)
param_128f.set('R', 10)
param_128f.set('Senc', 160)
param_128f.set('BETA', 1)
param_128f.set('ELL', 1280)
param_128f.set('Lke', 448)
param_128f.set('Lenc', 832)
param_128f.set('TAU', 16)
param_128f.set('W_GRIND', 8)
param_128f.set('T_OPEN', 130)
param_128f.set('SIG_SIZE', 6244)
param_128f.set('PK_SIZE', 32)
param_128f.set('SK_SIZE', 32)
param_128f.set('OWF_INPUT_SIZE', 16)
param_128f.set('OWF_OUTPUT_SIZE', 16)
parameter_sets += param_128f

# parameter set: VHDSA-192s
param_192s = configuration_data()
param_192s.set('PARAM', '192S')
param_192s.set('PARAM_L', '192s')
param_192s.set('LAMBDA', 192)
param_192s.set('Nst', 4)
param_192s.set('Ske', 32)
param_192s.set('R', 12)
param_192s.set('Senc', 192)
param_192s.set('BETA', 2)
param_192s.set('ELL', 2496)
param_192s.set('Lke', 448)
param_192s.set('Lenc', 1024)
param_192s.set('TAU', 16)
param_192s.set('W_GRIND', 12)
param_192s.set('T_OPEN', 180)
param_192s.set('SIG_SIZE', 11692)
param_192s.set('PK_SIZE', 48)
param_192s.set('SK_SIZE', 40)
param_192s.set('OWF_INPUT_SIZE', 16)
param_192s.set('OWF_OUTPUT_SIZE', 32)
parameter_sets += param_192s

# parameter set: VHDSA-192f
param_192f = configuration_data()
param_192f.set('PARAM', '192F')
param_192f.set('PARAM_L', '192f')
param_192f.set('LAMBDA', 192)
param_192f.set('Nst', 4)
param_192f.set('Ske', 32)
param_192f.set('R', 12)
param_192f.set('Senc', 192)
param_192f.set('BETA', 2)
param_192f.set('ELL', 2496)
param_192f.set('Lke', 448)
param_192f.set('Lenc', 1024)
param_192f.set('TAU', 24)
param_192f.set('W_GRIND', 8)
param_192f.set('T_OPEN', 180)
param_192f.set('SIG_SIZE', 15356)
param_192f.set('PK_SIZE', 48)
param_192f.set('SK_SIZE', 40)
param_192f.set('OWF_INPUT_SIZE', 16)
param_192f.set('OWF_OUTPUT_SIZE', 32)
parameter_sets += param_192f

# parameter set: VHDSA-256s
param_256s = configuration_data()
param_256s.set('PARAM', '256S')
param_256s.set('PARAM_L', '256s')
param_256s.set('LAMBDA', 256)
param_256s.set('Nst', 4)
param_256s.set('Ske', 52)
param_256s.set('R', 14)
param_256s.set('Senc', 224)
param_256s.set('BETA', 2)
param_256s.set('ELL', 3104)
param_256s.set('Lke', 672)
param_256s.set('Lenc', 1216)
param_256s.set('TAU', 22)
param_256s.set('W_GRIND', 6)
param_256s.set('T_OPEN', 260)
param_256s.set('SIG_SIZE', 21176)
param_256s.set('PK_SIZE', 48)
param_256s.set('SK_SIZE', 48)
param_256s.set('OWF_INPUT_SIZE', 16)
param_256s.set('OWF_OUTPUT_SIZE', 32)
parameter_sets += param_256s

# parameter set: VHDSA-256f
param_256f = configuration_data()
param_256f.set('PARAM', '256F')
param_256f.set('PARAM_L', '256f')
param_256f.set('LAMBDA', 256)
param_256f.set('Nst', 4)
param_256f.set('Ske', 52)
param_256f.set('R', 14)
param_256f.set('Senc', 224)
param_256f.set('BETA', 2)
param_256f.set('ELL', 3104)
param_256f.set('Lke', 672)
param_256f.set('Lenc', 1216)
param_256f.set('TAU', 32)
param_256f.set('W_GRIND', 8)
param_256f.set('T_OPEN', 260)
param_256f.set('SIG_SIZE', 26996)
param_256f.set('PK_SIZE', 48)
param_256f.set('SK_SIZE', 48)
param_256f.set('OWF_INPUT_SIZE', 16)
param_256f.set('OWF_OUTPUT_SIZE', 32)
parameter_sets += param_256f

# also make data from all parameter sets available "globally"
parameter_data = configuration_data()
foreach parameter_set : parameter_sets
  parameter_set_prefix = 'VHDSA_@0@'.format(parameter_set.get('PARAM'))
  foreach key : parameter_set.keys()
    parameter_data.set('@0@_@1@'.format(parameter_set_prefix, key), parameter_set.get(key))
  endforeach
endforeach
parameter_data.set('VHDSA_128_LAMBDA', 128)
parameter_data.set('VHDSA_192_LAMBDA', 192)
parameter_data.set('VHDSA_256_LAMBDA', 256)

# dependencies
openssl = dependency('openssl', required: get_option('openssl'))
boost_program_options = dependency('boost', required: get_option('benchmarks'), modules: ['program_options'])
threads = dependency('threads')
valgrind = dependency('valgrind', required: get_option('valgrind'))
valgrind_exec = find_program('valgrind', required: valgrind.found())

build_dependencies = []
if openssl.found()
  build_dependencies += [openssl]
  defines += '-DHAVE_OPENSSL'
endif

valgrind_defines = []
if valgrind.found() and valgrind_exec.found()
  valgrind_defines += '-DWITH_VALGRIND'
endif

pymod = import('python')
python = pymod.find_installation('python3', required: true)

if host_machine.system() == 'windows'
  # require new enough Windows for bcrypt to be available
  defines += '-D_WIN32_WINNT=0x0601'
  build_dependencies += cc.find_library('bcrypt')
endif

# catch2 for benchmarks
if get_option('benchmarks').enabled() and get_option('catch2').enabled()
  libcatch2 = static_library(
    'catch2',
    files(
      join_paths(
        meson.project_source_root(),
        'catch2',
        'extras',
        'catch_amalgamated.cpp',
      ),
    ),
  )
  catch2 = declare_dependency(
    link_with: libcatch2,
    include_directories: include_directories(join_paths('catch2', 'extras')),
  )
endif

# generate config.h
config_header = configure_file(output: 'config.h', output_format: 'c', configuration: conf_data)
defines += '-DHAVE_CONFIG_H'

# generate parameters.h
parameters_header = configure_file(output: 'parameters.h', output_format: 'c', configuration: parameter_data)

vhdsa_128_data = configuration_data()
vhdsa_128_data.set('SSS', '128')
vhdsa_aes_128 = configure_file(
  output: 'vhdsa_aes_128.c',
  input: 'vhdsa_aes.c.in',
  configuration: vhdsa_128_data,
)
vhdsa_192_data = configuration_data()
vhdsa_192_data.set('SSS', '192')
vhdsa_aes_192 = configure_file(
  output: 'vhdsa_aes_192.c',
  input: 'vhdsa_aes.c.in',
  configuration: vhdsa_192_data,
)
vhdsa_256_data = configuration_data()
vhdsa_256_data.set('SSS', '256')
vhdsa_aes_256 = configure_file(
  output: 'vhdsa_aes_256.c',
  input: 'vhdsa_aes.c.in',
  configuration: vhdsa_256_data,
)

include_directories = [include_directories('.')]

# source files
vhdsa_sources = files(
  'aes.c',
  'bavc.c',
  'compat.c',
  'vhdsa_impl.c',
  'fields.c',
  'instances.c',
  'owf.c',
  'random_oracle.c',
  'universal_hashing.c',
  'utils.c',
  'vole.c',
) + [
  config_header,
  parameters_header,
  vhdsa_aes_128,
  vhdsa_aes_192,
  vhdsa_aes_256,
]

if has_usable_builtin == false
  vhdsa_sources += ['cpu.c']
endif

# header files to install
headers = files(
  'vhdsa.h',
  'vhdsa_defines.h',
)

# SHA3 implementation
sha3 = get_option('SHA3')
if sha3 == 'auto'
  if cc.sizeof('void*') == 4
    sha3 = 'plain32'
  else
    sha3 = 'opt64'
  endif
endif
include_directories += [include_directories('sha3'), include_directories('sha3/' + sha3)]

if sha3 == 's390-cpacf'
  defines += '-DWITH_SHAKE_S390_CPACF'
else
  defines += '-DWITH_KECCAK_X4'
  vhdsa_sources += files(
    'sha3/KeccakHash.c',
    'sha3/KeccakHashtimes4.c',
    'sha3/KeccakSponge.c',
    'sha3/KeccakSpongetimes4.c',
  )

  if sha3 == 'avx2'
    vhdsa_sources += files(
      'sha3/avx2/KeccakP-1600-AVX2.s',
      'sha3/avx2/KeccakP-1600-times4-SIMD256.c',
    )
  elif sha3 == 'opt64'
    vhdsa_sources += files(
      'sha3/opt64/KeccakP-1600-opt64.c',
      'sha3/opt64/KeccakP-1600-times4-on1.c',
    )
  elif sha3 == 'plain32'
    vhdsa_sources += files(
      'sha3/plain32/KeccakP-1600-inplace32BI.c',
      'sha3/plain32/KeccakP-1600-times4-on1.c',
    )
  elif sha3 == 'armv8a-neon'
    vhdsa_sources += files(
      'sha3/armv8a-neon/KeccakP-1600-armv8a-neon.s',
      'sha3/opt64/KeccakP-1600-times4-on1.c',
    )
  endif

  # 32 bit ARM is in general unhappy with unaligned access, hence disable it in this case
  if host_machine.cpu_family() == 'arm'
    defines += '-DDISABLE_MISALIGNED_ACCESS=1'
  endif
endif

# generate files for parameter sets
foreach parameter_set : parameter_sets
  headers += configure_file(
    input: 'vhdsa_param.h.in',
    output: 'vhdsa_@0@.h'.format(parameter_set.get('PARAM_L')),
    configuration: parameter_set,
  )
  vhdsa_sources += configure_file(
    input: 'vhdsa_param.c.in',
    output: 'vhdsa_@0@.c'.format(parameter_set.get('PARAM_L')),
    configuration: parameter_set,
  )
endforeach

# static vhdsa library
libvhdsa_no_random_static = static_library(
  'vhdsa_no_random',
  vhdsa_sources,
  dependencies: build_dependencies + [valgrind],
  include_directories: include_directories,
  c_args: defines
  + c_flags
  + ['-DVHDSA_STATIC', '-DVHDSA_EXPORT=', '-DVHDSA_TESTS']
  + valgrind_defines,
)
libvhdsa_no_random_static_dependency = declare_dependency(
  link_with: libvhdsa_no_random_static,
  include_directories: include_directories,
  compile_args: ['-DVHDSA_EXPORT='],
)

libvhdsa_static = static_library(
  'vhdsa',
  files('randomness.c'),
  dependencies: build_dependencies + [libvhdsa_no_random_static_dependency, valgrind],
  include_directories: include_directories,
  c_args: defines
  + c_flags
  + ['-DVHDSA_STATIC', '-DVHDSA_EXPORT=', '-DVHDSA_TESTS']
  + valgrind_defines,
)
libvhdsa_static_dependency = declare_dependency(
  link_with: libvhdsa_static,
  include_directories: include_directories,
  compile_args: ['-DVHDSA_EXPORT='],
)

# shared library
visibility_define = []
if host_machine.system() == 'windows'
  visibility_define += '-DVHDSA_EXPORT=__declspec(dllexport)'
elif cc.has_function_attribute('visibility:default')
  visibility_define += '-DVHDSA_EXPORT=__attribute__((visibility("default")))'
endif
libvhdsa = shared_library(
  'vhdsa',
  vhdsa_sources + files('randomness.c'),
  dependencies: build_dependencies,
  include_directories: include_directories,
  c_args: defines + c_flags + visibility_define,
  link_args: linker_flags,
  install: true,
  version: '1.0.0',
  gnu_symbol_visibility: 'hidden',
)
libvhdsa_dependency = declare_dependency(link_with: libvhdsa, include_directories: include_directories)

# install headers
install_headers(headers)

# install pkg config file
pkg = import('pkgconfig')
pkg.generate(name: 'vhdsa', description: 'VHDSA signature scheme', libraries: [libvhdsa])

# SUPERCOP / NIST files
all_names = []
foreach parameter_set : parameter_sets
  all_names += 'vhdsa_@0@'.format(parameter_set.get('PARAM_L'))
  subdir('vhdsa_@0@'.format(parameter_set.get('PARAM_L')))
endforeach
run_target(
  'prepare_nist',
  command: [
    python,
    [
      join_paths(meson.project_source_root(), 'tools', 'prepare_nist.py'),
      meson.project_source_root(),
      meson.project_build_root(),
      join_paths(meson.project_build_root(), 'submission'),
    ]
    + all_names,
  ],

)

# subdir('doc')
# subdir('tests')
# subdir('tools')