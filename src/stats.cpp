/* Copyright 2020 Luc Bournaud (aka DevilishSpirits)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "stats.hpp"
#include "connection.hpp"
#include "request-workers.hpp"
fcgi::InternalStats fcgi::internal_stats;

fcgi::ConnectionStats::ConnectionStats(fcgi::Connection &connection)
{
	id = connection.fd;
	active_requests = connection.request_actives;
	socket_insane = connection.socket_insane;
}
void fcgi::GlobalStats::update(void)
{
	socket_listening = fcgi::internal_stats.socket_listening;
	quick_shutdown = fcgi::internal_stats.quick_shutdown;

	workers_online = fcgi::worker::workers_online;
	workers_sleeping = fcgi::worker::workers_sleeping;
	workers_backlog = fcgi::worker::workers_backlog;
	
	connections.clear();
	{
		std::lock_guard<std::mutex> lock(fcgi::Connection::global_list_mutex);
		for (fcgi::Connection& connection: fcgi::Connection::global_list)
			connections.emplace_back(connection);
	}
}
