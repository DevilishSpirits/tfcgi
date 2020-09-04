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
#include "request-workers.hpp"
#include <fcgi.hpp>
#include "connection.hpp"
#include <errno.h>
#include <sys/socket.h>
#include <unistd.h>

int main(int argc, char** argv)
{
	int result = fcgi::startup(argc,argv);
	if (result)
		return result;
	//syslog(LOG_DEBUG,"Application initialized. Initializing framework...");
	// Raise socket_listening before starting workers (else they die immediately)
	fcgi::internal_stats.socket_listening = true;
	std::vector<fcgi::worker> threads;
	// TODO Spawn a configurable number
	threads.resize(std::thread::hardware_concurrency() * 5);
	//syslog(LOG_NOTICE,"Ready and accepting connections");
	while (fcgi::internal_stats.socket_listening) {
		// TODO Find a way to stop
		int new_sock = accept(0,NULL/* TODO struct sockaddr *restrict address */,NULL /* TODO socklen_t *restrict address_len*/);
		// TODO Perform basic address filtering
		if (new_sock < 0)
			switch (errno) {
				case EMFILE:
				case ENFILE:
					//syslog(LOG_CRIT,"accept() failed: %m. Retrying in 100ms");
					std::this_thread::sleep_for(std::chrono::milliseconds(100));
				case EAGAIN:
				case ECONNABORTED:
				case EINTR:
					continue;
				default: {
					//syslog(LOG_ERR,"accept() failed: %m.");
					fcgi::Connection::close_listening_socket();
				} break;
			}
		else fcgi::Connection::create_connection(new_sock);
	}
	//syslog(LOG_NOTICE,"No longer accepting connections");
	fcgi::begin_shutdown(fcgi::internal_stats.quick_shutdown);
	for (std::thread &thread: threads)
		thread.join();
	//syslog(LOG_INFO,"All threads are gone");
	return fcgi::shutdown(fcgi::internal_stats.quick_shutdown);
}
