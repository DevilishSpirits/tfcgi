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
#include "request-workers.hpp"
#include "stats.hpp"
#include "connection.hpp"
#include <fcgi.hpp>
static std::mutex              queue_mutex;
static std::condition_variable queue_bell;
static std::queue<std::reference_wrapper<fcgi::Request>> queue;

void fcgi::worker::push_request(Request &request)
{
	std::lock_guard<std::mutex> lock(queue_mutex);
	queue.push(std::ref(request));
	queue_bell.notify_one();
}
static fcgi::Request& pop_request(void)
{
	std::unique_lock<std::mutex> lock(queue_mutex);
	while (queue.empty()) {
		queue_bell.wait(lock);
	}
	fcgi::Request& request = queue.front();
	queue.pop();
	return request;
}
void fcgi::worker::thread_func(void)
{
	while (internal_stats.socket_listening || internal_stats.socket_active) {
		// Pop!
		fcgi::Request& request = pop_request();
		
		// Prepare the end-request packet
		struct {
			fcgi::Header header;
			fcgi::EndRequestBody body;
		} end_request_packet = {{1,fcgi::RecType::END_REQUEST,htons(request.requestId),htons(sizeof(fcgi::EndRequestBody)),0,0},{htonl(0xA990EEEE),fcgi::EndRequestProtoStatus::REQUEST_COMPLETE}};
		// Call the application
		try {
			switch (request.begin_request().role) {
				#if FCGI_RESPONDER
				case fcgi::BeginRequestRole::RESPONDER: {
					end_request_packet.body.appStatus = fcgi::respond(request);
				} break;
				#endif
				#if FCGI_AUTHORIZER
					#error fcgi::BeginRequestRole::AUTHORIZER not supported
				#endif
				#if FCGI_FILTER
					#error fcgi::BeginRequestRole::FILTER not supported right-now
				#endif
				default: {
					end_request_packet.body.protocolStatus = fcgi::EndRequestProtoStatus::UNKNOWN_ROLE;
					end_request_packet.body.appStatus      = request.begin_request().role;
					#if FCGI_CUSTOM_ROLES
					fcgi::custom(request,end_request_packet.body);
					#endif
					if (end_request_packet.body.protocolStatus == fcgi::EndRequestProtoStatus::UNKNOWN_ROLE)
						if (request.cerr.good())
							request.cerr << std::endl << "Unknow role " << request.begin_request().role << " is not suported." << std::endl;
				} break;
			}
		} catch (const std::exception& e) {
			//syslog(LOG_ERR,"Exception while handling request : %s",e.what());
			if (request.cerr.good())
				request.cerr << std::endl << "Exception occured : " << e.what() << std::endl;
		} catch (...) {
			//syslog(LOG_ERR,"Unknow exception while handling request.");
			if (request.cerr.good())
				request.cerr << std::endl << "An unknow exception occured." << std::endl;
		}
		// Close streams
		request.cout.close();
		request.cerr.close();
		// Set the packet status as finished
		request.connection.send_packet(reinterpret_cast<char*>(&end_request_packet),sizeof(end_request_packet)); // TODO Error checks
		request.connection.request_actives--;
	}
}
