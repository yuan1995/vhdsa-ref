sources = configure_file(
    input: '../crypto_sign.c.in',
    output: 'crypto_sign.c',
    configuration: param_256s
)
headers = [
    configure_file(
        input: '../crypto_sign.h.in',
        output: 'crypto_sign.h',
        configuration: param_256s
    ),
    configure_file(
        input: '../crypto_sign.h.in',
        output: 'api.h',
        configuration: param_256s
    )
]

libvhdsa_256s = static_library('vhdsa_256s',
  sources,
  dependencies: libvhdsa_static_dependency,
  include_directories: include_directories,
  c_args: defines + c_flags
)
libvhdsa_256s_dependency = declare_dependency(
  link_with: libvhdsa_256s,
  include_directories: include_directories
)
if openssl.found()
  tv_sources = files(
    join_paths(meson.project_source_root(), 'randomness.c'),
    join_paths(meson.project_source_root(), 'tools', 'rng.c'),
    join_paths(meson.project_source_root(), 'tools', 'PQCgenKAT_sign.cpp')
  )
  test_vector_generator = executable('vhdsa_256s_test_vectors', [sources] + tv_sources,
    dependencies: [libvhdsa_no_random_static_dependency, openssl],
    include_directories: include_directories,
    c_args: defines + c_flags + ['-DHAVE_RANDOMBYTES'],
    cpp_args: defines + cpp_flags + ['-DHAVE_RANDOMBYTES'],
    link_args: linker_flags,
    override_options: ['b_lto=false'],
  )
endif
if boost_program_options.found() and get_option('benchmarks').enabled()
  bench_sources = files(join_paths(meson.project_source_root(), 'tools', 'bench.cpp'))
  bench = executable('vhdsa_256s_bench', bench_sources,
    dependencies: [libvhdsa_256s_dependency, boost_program_options, threads],
    include_directories: include_directories,
    c_args: defines + c_flags,
    cpp_args: defines + cpp_flags,
    link_args: linker_flags
  )
endif
test_sources = files(join_paths(meson.project_source_root(), 'tests', 'api_test.c'))
vhdsa_256s_test = executable('vhdsa_256s_api_test', test_sources,
  dependencies: [libvhdsa_256s_dependency, valgrind],
  include_directories: include_directories,
  c_args: defines + c_flags + valgrind_defines,
  link_args: linker_flags,
  override_options: ['b_lto=false'],
)
test('vhdsa_256s_api_test', vhdsa_256s_test,
  timeout: 6000,
)
if valgrind.found() and valgrind_exec.found()
  test('vhdsa_256s_api_test_ct', valgrind_exec,
    args: ['-q', '--error-exitcode=1', '--track-origins=yes', vhdsa_256s_test],
    timeout: 6000,
  )
endif
if valgrind_exec.found()
  custom_target('vhdsa_256s_memory_usage',
    command: [valgrind_exec, '-q', '--error-exitcode=1', '--tool=massif', '--stacks=yes', '--massif-out-file=@OUTPUT@', vhdsa_256s_test],
    output: 'vhdsa_256s.massif',
    depends: [vhdsa_256s_test],
    install: false,
    build_always_stale: true,
    build_by_default: false,
  )
endif
if get_option('catch2').enabled() and get_option('benchmarks').enabled()
  bench_sources = files(
    join_paths(meson.project_source_root(), 'tools', 'bench_c2.cpp'),
  )
  bench_catch = executable('vhdsa_256s_bench_c2', bench_sources,
    dependencies: [libvhdsa_256s_dependency, boost_program_options, catch2],
    include_directories: include_directories,
    c_args: defines + c_flags,
    cpp_args: defines + cpp_flags,
    link_args: linker_flags
  )
endif
