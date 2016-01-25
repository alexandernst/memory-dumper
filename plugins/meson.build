swf = shared_library(
	'swf', 'swf.cpp',
	include_directories: src_dir,
	dependencies: cppbitstringstatic
)

cwf = shared_library(
	'cwf', 'cwf.cpp',
	include_directories: src_dir,
	dependencies: [zlib, cppbitstringstatic]
)

zwf = shared_library(
	'zwf', 'zwf.cpp',
	include_directories: src_dir,
	dependencies: [lzma, cppbitstringstatic]
)

pdf = shared_library(
	'pdf', 'pdf.cpp',
	include_directories: src_dir,
	dependencies: cppbitstringstatic
)
