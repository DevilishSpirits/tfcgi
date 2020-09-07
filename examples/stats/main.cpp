#include <fcgi.hpp>
#include "include/fcgi-stats.hpp"
int fcgi::startup(int argc, char** argv)
{
	return 0;
}
int fcgi::respond(Request &request)
{
	if (request.params["REQUEST_METHOD"] != "GET") { // TODO convert to upper-case
		request.sendStatus(405);
		request.sendHeader("Allow","GET");
		request.finishHeaders();
		return 0;
	}
	return fcgi::examples::stats::respond_json(request);
}
void fcgi::begin_shutdown(bool quick)
{
}
int fcgi::shutdown(bool quick)
{
	return 0; 
}
