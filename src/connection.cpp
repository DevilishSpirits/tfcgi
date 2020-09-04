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

#include "connection.hpp"
#include "ipacket.hpp"
#include "stats.hpp"
#include <unistd.h>
#include <system_error>
#include <errno.h>
#include <syslog.h>
#include <thread>

std::list<fcgi::Connection> fcgi::Connection::global_list;
std::mutex                  fcgi::Connection::global_list_mutex;

void fcgi::Connection::recv_all(char *data, size_t size)
{
	while (size) {
		ssize_t got = read(fd,data,size);
		if (got > 0) {
			size -= got;
			data   += got;
		} else {
			socket_insane = false;
			if (got == 0)
				errno = ECONNRESET;
			switch (errno) {
				case ENOBUFS:
				case EINTR:continue; // Try again
				default:
					syslog(LOG_ERR,"conn%d:%m",fd);
				case ECONNRESET:break;
					//syslog(LOG_DEBUG,"conn%d:%m",fd);
			}
			return;
		}
	}
}
void fcgi::Connection::send_packet(const char *buffer, size_t size)
{
	std::lock_guard<std::mutex> lock(out_mutex);
	while (size) {
		ssize_t sent = send(fd,buffer,size,MSG_NOSIGNAL);
		if (sent >= 0) {
			size -= sent;
			buffer   += sent;
		} else switch (errno) {
			case ENOBUFS:
			case ENETUNREACH:
			case ENETDOWN:
			case EINTR:break; // Try again
			case ECONNRESET:return; // TODO I'm sure we can do something better
			case EPIPE:return; // TODO I'm sure we can do something better
			default:
				throw std::system_error(errno,std::system_category());
		}
	}
}

void fcgi::Connection::thread_func(std::list<fcgi::Connection>::iterator iterator)
{
	fcgi::Connection& connection = *iterator;
	while (connection.socket_insane) {
		// Read the packet
		std::unique_ptr<fcgi::ipacket> packet = std::make_unique<fcgi::ipacket>(connection);
		const int requestId = packet->header().requestId;
		// Check if something gone wrong before continuing
		if (!connection.socket_insane) {
			break;
		}
		// Handle the packet
		if (requestId) {
			// Allocate storage if needed
			if (connection.requests.size() < requestId)
				connection.requests.resize(requestId);
			// Allocate the request if it not already
			std::unique_ptr<fcgi::Request> &request_ptr = connection.requests[requestId - 1];
			if (!request_ptr)
				request_ptr = std::make_unique<fcgi::Request>(connection,requestId);
			// Take the control
			request_ptr->handle_packet(packet);
		} else switch (packet->header().type) {
			case fcgi::RecType::GET_VALUES: {
				connection.get_value_handler.handle_packet(packet);
			} break;
			default: {
				struct {
					fcgi::Header header;
					fcgi::UnknownTypeBody body;
				} end_type_packet = {{1,fcgi::RecType::UNKNOWN_TYPE,htons(requestId),htons(sizeof(fcgi::UnknownTypeBody)),0,0},{packet->header().type,0,0,0,0,0,0,0}};
				connection.send_packet(reinterpret_cast<char*>(&end_type_packet),sizeof(end_type_packet));
				syslog(LOG_WARNING,"conn%d:Unknow packet type %d (requestId 0)",connection.fd,packet->header().type);
			} break;
		}
	}
	// Destroy much packet as possible
	for (std::unique_ptr<fcgi::Request> &request: connection.requests)
		if (request)
			switch (request->state) {
				case fcgi::Request::State::READING_PARAMS:
					// This is an active packet that won't be processed
					connection.request_actives--;
				case fcgi::Request::State::READY:
					// Prematuraly destroy the request,
					request.reset();
			}
	// Wait for remaining requests
	while (connection.request_actives)
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	// Remove myself
	{
		std::lock_guard<std::mutex> lock(fcgi::Connection::global_list_mutex);
		fcgi::Connection::global_list.erase(iterator);
	}
}

void fcgi::Connection::create_connection(int fd)
{
	std::lock_guard<std::mutex> lock(fcgi::Connection::global_list_mutex);
	fcgi::Connection::global_list.emplace_front(fd);
	std::thread(fcgi::Connection::thread_func,fcgi::Connection::global_list.begin()).detach();
}
fcgi::Connection::Connection(int fd) : request_actives(0), socket_insane(true) /* <- So much ! */, get_value_handler(get_value_result_stream), get_value_result_stream(*this,0,fcgi::RecType::GET_VALUES_RESULT), fd(fd)
{
	fcgi::internal_stats.socket_active++;
	//syslog(LOG_INFO,"conn%d: New connection",fd);
}
fcgi::Connection::~Connection(void)
{
	// Destroy all packets before socket close
	requests.clear();
	get_value_result_stream.close();
	// Close the socket
	//syslog(LOG_INFO,"conn%d: Close connection",fd);
	close(fd);
	fcgi::internal_stats.socket_active--;
}

void fcgi::Connection::close_listening_socket(void)
{
	bool expected = true;
	if (fcgi::internal_stats.socket_listening.compare_exchange_strong(expected,false,std::memory_order_relaxed)) {
		do {
			errno = 0;
			close(0);
		} while (errno == EINTR);
		if (errno)
			syslog(LOG_WARNING,"Error when closing FastCGI listening socket: %m.");
		else syslog(LOG_DEBUG,"FastCGI listening socket closed.");
	}
}

void fcgi::Connection::GetValueHandler::new_key(fcgi::NVP::string_view_pair pair)
{
	if (pair.first == "FCGI_MPXS_CONNS")
		fcgi::NVP::serialize(output_stream,"FCGI_MPXS_CONNS","1");
	else if (pair.first == "FCGI_MAX_REQS")
		// TODO Mak that programable
		fcgi::NVP::serialize(output_stream,"FCGI_MAX_REQS",std::to_string(std::thread::hardware_concurrency() * 20));
	else if (pair.first == "FCGI_MAX_CONNS")
		// TODO Mak that programable
		fcgi::NVP::serialize(output_stream,"FCGI_MAX_CONNS",std::to_string(std::thread::hardware_concurrency()));
	else syslog(LOG_INFO,"Unknow GET_VALUES key %s",pair.first.data());
	syslog(LOG_INFO,"Got GET_VALUES key %s",pair.first.data());
}
void fcgi::Connection::GetValueHandler::handle_packet(std::unique_ptr<fcgi::ipacket> &packet)
{
	syslog(LOG_INFO,"Got GET_VALUES key!");
	if (fcgi::NVP::Deserializer::feed(packet)) {
		syslog(LOG_INFO,"Upstream closed GET_VALUES stream");
		output_stream.close();
	}
}
