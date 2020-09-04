# Tiny FastCGI tutorial
You should see examples, they are heavily commented and self-explanatory.
An important things, **TFCGI have to be built statically in your app** due to
some configuration specific to the project and the ABI instability.

Before that, ask yourself if tfcgi is the right framework :

 - **You** must synchronize all global/shared variables **you** create.

 - You don't have control on the main-loop nor worker threads.
 
 - You serve requests in parallel.

If this is not a problem, enjoy tfcgi and save your time.

## 0. Preparing the environment
Clone the Git repository somewhere and try to build the library :

```sh
	git clone repo
	meson -C repo repo/build
	ninja -C repo/build
```

If the build suceed it's okay, now you can `rm -r repo/build`.

## 1. Boilerplate code
Create a new directory for your project, create a `subproject` directory and
inside create a link to the root of this Git.

```sh
mkdir project
cd project
mkdir subprojects
ln -s *path_to_fcgi_git* subprojects/tfcgi
touch main.cpp    # The beginning
touch meson.build # Instructions for the Meson build-system
```

We are using a [Meson subproject](https://mesonbuild.com/Subprojects.html) to
build to the library.

Now open `meson.build` and add this content :

```meson
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
```

Now you can build this empty project and get a linker error for 4 functions.

```sh
	# cd to_the_project_root
	meson build # <- RUN ONLY ONCE ! It configure the project
	ninja -C build # <- Always use this command after configuring Meson
```

## 2. Application (de)initialization
Now interesting things begin. You won't write a `main` but implement callbacks
defined in `fcgi.hpp`. This framework handle the main-loop and multi-threading.
This have many implications, firstly anything that's global must be synchronized
by yourself and you cannot precisely control when the server stop. But this
greatly simplify things and you can focus on the essential, serving requests.

Now open `main.cpp` and append `#include <fcgi.hpp>` to start.

Firstly you have to implement `int startup(int argc, char** argv);`, it's really
the first function called and where you initialize your application and parse
command-line. If initialization fail, return a non-zero value, it's the program
exit code. Return zero to continue startup, the framework will start and listen.

```cpp
int fcgi::startup(int argc, char** argv)
{
	return 0; // Nothing for now
}
```

After starting-up, soon or late the server will shutdown, you have 2 callbacks :
`void begin_shutdown(bool quick);` and `int shutdown(bool quick);`.
`begin_shutdown` is called early in the shutdown process while requests are
still processing, you can raise a flag to prevent some operations to begin.
When all request and connections are gone, `shutdown` is called and you destroy
what you built in `startup` and his result is the program exit code.
The only difference is when those function are called, by default you would do
most deinitialization in `shutdown` since you are sure that no thread (unless
your ones) will access globals. `begin_shutdown` is useful to deny long running
requests. The `quick` param is true when you should fastly end the server, like
upon `SIGTERM`.

```cpp
void fcgi::begin_shutdown(bool quick)
{
	// Since the startup is fairly boring ...
}
int fcgi::shutdown(bool quick)
{
	return 0; // ... this part is no more fun.
}
```

## 3. Serving
Now the thrilling part, serving requests ! 

You'll deal with `fcgi::Request&`'s, they bundle FastCGI streams (`cin`, `cout`,
`cerr` and `cdata`) and parameters (`params`). They are passed in callback
params and are invalidated after return (and *normally* reused). FastCGI streams
are true C++ stream and `cout`/`cerr` can be `close()`.
Note that **you really SHOULD NOT close `cerr`**, if an exception occur while
running the request, details are printed to this stream. But if it's closed, we
cannot send those precious details. This may not be possible in the future to
prevent users from this really stupid mistake.

Then there different FastCGI roles :

 - `responder` generate a request sent to the client, it's the *normal* one.

 - `authorizer` decide weather a request is allowed to complete.
 
 - `filter` filter `cin` using `cdata` and params.

**Note!** Don't forget to `add_global_arguments()` in `meson.build` to enable
the role you want.

**WARNING!** Reading an implicitely closed stream (oftenly `cdata`) will cause a
**dead-lock** !!!

### Some history
FastCGI is built on CGI. A CGI app have an environment and some streams that
were the process environment and standard streams, a process was spawn upon each
requests. The overhead of spawning a process was to high for big websites and
FastCGI was a more efficient alternative. Differences with CGI here are that :

 - Environment is passed in the `params` dictionary of the `request` object
 
 - Standard I/O are wrapped in the `request` object (`std::cin` → `request.cin`)
 
 - You may not call `exit()`, you must return from the pseudo *main()*.

### Basics
The CGI standard define many environment variables :

 - `CONTENT_LENGTH` of `cin`, an application must report error if it read less
 
 - `REQUEST_METHOD` of the request (GET, POST, ...)
 
 - `QUERY_STRING` after the `?` in the URL of the request
 
 - Many more... Refer to your HTTP server documentation

A CGI response look-like an HTTP response without the first line. A CGI app must
reply with either a `Content-Type`, the HTTP `Status` or `Location` header.
Replying with a `Location` imply a redirection using the *302 Found* HTTP status
code, a `Content-Type` imply *200 Success*. You may override the status by
explicitely setting the `Status` header. You can add custom HTTP headers just by
appending them as-is. Some examples of replies :

```
Content-Type: text/plain
Cache-Control: public
Expires: Tue, 19 Jan 2038 03:14:07 GMT

Hello world!
```

```
Location: https://example.com/
Status: 301 Moved
```

```cpp
// A complete main.cpp
#include <fcgi.hpp>
int fcgi::startup(int argc, char** argv)
{
	return 0;
}
int fcgi::respond(fcgi::Request &request)
{
	request.sendContent_Type("text/plain");
	request.sendHeader("Cache-Control","public");
	request.sendHeader("Expires","Tue, 19 Jan 2038 03:14:07 GMT");
	request.finishHeaders();
	/* After function inlining :
	 * request.cout << "Content-Type: text/plain\n";
	 * request.cout << "Cache-Control: public\n";
	 * request.cout << "Expires: Tue, 19 Jan 2038 03:14:07 GMT\n";
	 * request.cout << "\n";
	 *
	 * You can do either way and even mix those. But theses are stateless
	 * convenience wrapper, don't send datas before finishHeaders().
	 */
	request.cout << "Hello world!";
	request.cout.close(); // Optional, implicit when the function return
	return 0;
}
void fcgi::begin_shutdown(bool quick)
{
}
int fcgi::shutdown(bool quick)
{
	return 0; 
}
```

Now you have a working FastCGI server, now go to the next section.

### Authorizer role
This request type is currently not supported. Handle it manually (section 3c).

### Filter role
This request type is currently not supported. Handle it manually (section 3c).

### Non-standard role
If a role is not handled by tfcgi, you can handle it in the 
`void fcgi::custom(Request &request, EndRequestBody &result);` callback.
The `result` is preset to `fcgi::EndRequestProtoStatus::UNKNOWN_ROLE` with the
application status code being the unknow role id.
If you exit with `fcgi::EndRequestProtoStatus::UNKNOWN_ROLE`, the framework
will log details in `cerr`.
Note that this catch request for all unhandled requests, including responder, 
authorizer and filtering ones, even when implementing those, always check the
role id.

## 4. Testing with an HTTP server
See your HTTP server documentation for FastCGI program. PHP being one of the
most popular FastCGI application, it's oftenly used as an example.

Here I'll do an example with [lighttpd](https://www.lighttpd.net/) because it's
very easy to use. Here a sample configuration tailored for the example that
route all traffic to a FastCGI backend.

```sh
# Simple FastCGI configuration example (lighttpd.conf)
server.modules += ("mod_fastcgi")
server.port = 8080
server.document-root = "/dev/null" # Or something less black-holed
fastcgi.server = ( "" =>
	((
		"socket" => "/tmp/tfcgi.sock", # You should change that
		"bin-path" => "absolute/path/to/the/executable",
		"check-local" => "disable"
	))
)
```

You can run the server with :

```sh
lighttpd -D -f /path/to/lighttpd.conf
```

Now browse to [your new website](http://localhost:8080/) !
