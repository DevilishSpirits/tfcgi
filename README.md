# Tiny FastCGI C++ pet framework

This is a minimalist multi-threaded FastCGI implementation. Unlike traditionnal
libraries, this one implement the `main()` and you have to implement 4 functions
in `fcgi.hpp` to have the magic works. Focus you on the service, not on the lib.
The API very simple, almost easy as the great old CGI with better performances.

This framework depend on C++17 and a restricted subset of POSIX. So while I test
it on my ArchLinux machine only, it should works on any UNIX-like system.

I use the [Meson Build system](https://mesonbuild.com).

This is a toy project that don't scale well currently and may not works.

## Using
*Go to `TUTORIAL.md` for a fast development tutorial.*

At start, the program must have a listening socket in stdin, stdout and stderr
are closed as said in the FastCGI standard. You should use the lighttpd's
[`spawn-fcgi`](https://redmine.lighttpd.net/projects/spawn-fcgi) or this
[systemd trick](https://b.mtjm.eu/fastcgi-systemd-socket-activation.html) to
spawn the process. Of course, if you have another way to spawn the process in a
FastCGI manner, you can use it.

You should stop the process with a `SIGTERM` or a `SIGQUIT`, currently it's like
`SIGKILL` but custom behavior may be implemented in a future version.
The framework itself can be `SIGKILL`ed with no problem (altough I don't he like
being killed this way... and you ?).
