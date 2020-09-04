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
