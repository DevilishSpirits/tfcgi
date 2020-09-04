project('project', 'cpp', default_options: ['cpp_std=c++17'])
# Many compilers don't enable C++ 17 by default. ↑ This way I enable it.

# Enable the responder role
add_global_arguments('-DFCGI_RESPONDER=1',language: 'cpp')
#add_global_arguments('-DFCGI_AUTHORIZER=1',language: 'cpp')
#add_global_arguments('-DFCGI_FILTER=1',language: 'cpp')
# Uncomment those lines to enable the FastCGI role you are interested as the
# API is different. If you just want to serve webpages and content let this as
# is.
#add_global_arguments('-DFCGI_CUSTOM_ROLES=1',language: 'cpp')
# If you want to handle yourself unknow roles, uncomment the line above.

# Import the framework as a subproject and the dependency object
tfcgi = subproject('tfcgi')
tfcgi_dep = tfcgi.get_variable('tfcgi_dep')

# Now add this dependency where it's used.
# The main executable is a good choice
executable('a_good_name', 'main.cpp', dependencies: tfcgi_dep)
