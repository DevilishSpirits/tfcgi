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
#include <fcgi/request.hpp>
#include "ipacket.hpp"
#include "request-workers.hpp"
#include "stats.hpp"

fcgi::Request::Request(Connection &connection, uint16_t requestId) : state(fcgi::Request::State::READY), requestId(requestId), connection(connection), cout(*this,fcgi::RecType::STDOUT), cerr(*this,fcgi::RecType::STDERR)
{
}

void fcgi::Request::handle_packet(std::unique_ptr<fcgi::ipacket> &a_packet)
{
	fcgi::ipacket &packet = *a_packet;
	switch (packet.header().type) {
		case fcgi::RecType::BEGIN_REQUEST: {
			if (state != fcgi::Request::State::READY) {
				// TODO The server is going crazy !
			} else {
				packet.recv_all(reinterpret_cast<char*>(&priv_begin_request),sizeof(priv_begin_request));
				cerr.reset();
				cout.reset();
				switch (priv_begin_request.role = static_cast<fcgi::BeginRequestRole>(ntohs(priv_begin_request.role))) {
					#if FCGI_RESPONDER
					case fcgi::BeginRequestRole::RESPONDER: {
						cin.reset();
						cdata.rdbuf(NULL);	
					} break;
					#endif
					#if FCGI_AUTHORIZER
					case fcgi::BeginRequestRole::AUTHORIZER: {
						cin.rdbuf(NULL);
						cdata.rdbuf(NULL);
					} break;
					#endif
					#if FCGI_FILTER
					case fcgi::BeginRequestRole::FILTER:
					#endif
					default: {
						cin.reset();
						cdata.reset();
					} break;
				}
				
				/* -- Request accepted -- */
				// Raise send EOF flags
				cout.internal_streambuf->send_eof = true;
				cerr.internal_streambuf->send_eof = true;
				// Reset params_deserializer
				params.clear();
				params_deserializer = std::make_unique<NVP::ToMap>(params);
				// Update stats
				state = fcgi::Request::State::READING_PARAMS;
				connection.request_actives++;
			}
		} break;
		case fcgi::RecType::ABORT_REQUEST: {
			// TODO We should do something
		} break;
		case fcgi::RecType::PARAMS: {
			if (params_deserializer) {
				try {
					if (params_deserializer->feed(a_packet)) {
						state = fcgi::Request::State::PENDING;
						fcgi::worker::push_request(*this);
						params_deserializer.reset();
					}
				} catch (fcgi::NVP::Deserializer::pair_size_error &e) {
					// Cancel the request
					cerr << e.what() << " Raise fcgi::limits::nvp_max_pair_size or fix the fat parameter.\n";
					cerr.close();
					cout.close();
					params_deserializer.reset();
					connection.send_packet<fcgi::EndRequestBody>(fcgi::RecType::END_REQUEST,requestId,htonl(0X9020816E),fcgi::EndRequestProtoStatus::REQUEST_COMPLETE,0,0,0);
					state = fcgi::Request::State::READY;
					connection.request_actives--;
				}
			} else {
				// TODO The server is going crazy !
			}
		} break;
		case fcgi::RecType::STDIN: {
			if (state != fcgi::Request::State::READY)
				cin.feed(a_packet);
		} break;
		case fcgi::RecType::DATA: {
			if (state != fcgi::Request::State::READY)
				cdata.feed(a_packet);
		} break;
		case fcgi::RecType::GET_VALUES: {
			// TODO
		} break;
		default: {
			connection.send_packet<fcgi::UnknownTypeBody>(fcgi::RecType::UNKNOWN_TYPE,requestId,packet.header().type,0,0,0,0,0,0,0);
			//syslog(LOG_WARNING,"conn%d:Unknow packet type %d for request %d",connection.fd,packet.header().type,requestId);
		} break;
	}
}
