#include "include/fcgi-stats.hpp"

void fcgi::examples::stats::dump_to_json(std::ostream& cout)
{
	fcgi::GlobalStats stats;
	stats.update();
	return dump_to_json(cout,stats);
}

void fcgi::examples::stats::dump_to_json(std::ostream& cout, fcgi::GlobalStats &stats)
{
	cout << "{\"socket_listening\": " << (stats.socket_listening ? "true" : "false")
	     << ",\"quick_shutdown\": " << (stats.quick_shutdown ? "true" : "false")
	     << ",\"workers_online\": " << stats.workers_online
	     << ",\"workers_sleeping\": " << stats.workers_sleeping
	     << ",\"workers_backlog\": " << stats.workers_backlog
	     << ",\"connections\": [";
	
	fcgi::ConnectionStats connection_totals;
	connection_totals.active_requests = 0;
	bool first_connection = true;
	for (fcgi::ConnectionStats &stat: stats.connections) {
		if (first_connection)
			first_connection = false;
		else cout << ',';
		cout << "{\"id\": " << stat.id
		     << ",\"active_requests\": " << stat.active_requests
		     << ",\"socket_insane\": " << (stats.quick_shutdown ? "true" : "false")
		     << "}";
		connection_totals.active_requests += stat.active_requests;
	}
	cout << "]}";
}

int fcgi::examples::stats::respond_json(fcgi::Request &request)
{
	fcgi::GlobalStats stats();
	request.sendContent_Type("application/json");
	request.finishHeaders();
	dump_to_json(request.cout);
	request.cout.close();
	return 0;
}
