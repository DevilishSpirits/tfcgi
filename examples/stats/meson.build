project('fcgixx-example-stats', 'cpp', default_options: ['cpp_std=c++17'])

# Enable the responder role
add_global_arguments('-DFCGI_RESPONDER=1',language: 'cpp')

# Create the executable using the tfcgi dependency object
app = executable('example-stats', 'main.cpp', 'stats.cpp', dependencies: subproject('tfcgi').get_variable('tfcgi_dep'))

# Configure the example lighttpd.conf
configure_file(input: 'lighttpd.conf.in', output: 'lighttpd.conf', configuration: {
	'EXAMPLE_ROOT': meson.current_source_dir(),
	'FASTCGI_EXEC': app.full_path(),
	'EXAMPLE_BUILD_ROOT': meson.current_build_dir(),
})
