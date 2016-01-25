project('memory-dumper', 'cpp', default_options: ['warnlevel=2', 'cpp_std=c++11'])

dl = find_library('dl')
zlib = find_library('z')
lzma = find_library('lzma')
cppbitstringstatic = find_library('cpp-bitstring-static', dirs: meson.source_root() + '/libs/cpp-bitstring/')

src_dir = include_directories('src')

subdir('plugins')

memorydumper = executable(
	'memory-dumper', 'src/MemoryDumper.cpp', 'src/MemoryDumper.cpp',
	dependencies: [dl, cppbitstringstatic]
)

test('memory-dumper', memorydumper)
