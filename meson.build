# Copyright 2020 Luc Bournaud (aka DevilishSpirits)
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#     http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

project('tfcgi', 'cpp', default_options: ['cpp_std=c++17'])

add_project_arguments('-Wno-switch', language: 'cpp')

cxx = meson.get_compiler('cpp')
if cxx.get_define('CHAR_BIT', prefix: '#include <climits>') != '8'
	# Just in case someone (like me) have a weird system
	error('CHAR_BIT must be 8')
endif

lighttpd_prog = find_program('lighttpd', required: false)

deps = [
	dependency('threads', required: false),
]
tfcgi_includes = include_directories('include')
src = [
	'src/connection.cpp',
	'src/iostream.cpp',
	'src/ipacket.cpp',
	'src/main.cpp',
	'src/name-value-pairs.cpp',
	'src/request.cpp',
	'src/request-workers.cpp',
	'src/stats.cpp',
]
tfcgi_lib = static_library('fcgixx', src, include_directories: tfcgi_includes, dependencies: deps)

tfcgi_dep = declare_dependency(link_with: tfcgi_lib, include_directories: tfcgi_includes)
