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
#pragma once
#include <fcgi/protocol.hpp>
#include <fcgi/request.hpp>
#include <fcgi/name-value-pairs.hpp>
#include <arpa/inet.h>
#include <list>
#include <mutex>
#include <memory>
#include <thread>
#include <atomic>
#include <condition_variable>
namespace fcgi {
	class ipacket;
	class Request;
	class worker;
	class ConnectionStats;
	class Connection {
		private:
			friend ipacket;
			friend Request;
			friend worker;
			friend ConnectionStats;
			/** Socket output mutex
			 *
			 * This mutex ensure isolation of the sending side of the socket to avoid
			 * protocol breakage.
			 */
			std::mutex out_mutex;
			
			/** Number of active requests
			 *
			 * This is the number of active requests on the FastCGI protocol. It's
			 * increased when receiving a BEGIN_REQUEST packet and decreased when we
			 * END_REQUEST a packet.
			 *
			 * When the socket is down, the connection thread poll this value and free
			 * the Connection when it reach zero.
			 */
			std::atomic<unsigned int> request_actives;
			/** True if we can read from the socket
			 *
			 * When a read error occur, this variable is set to false and the socket
			 * read direction is shutdown. The framework finish current requests then
			 * close the connection.
			 */
			bool socket_insane;
			
			/** Receive size bytes
			 *
			 * This function is not thead-safe and is only called from an #ipacket
			 * constructor
			 */
			void recv_all(char *data, size_t size);
			
			std::vector<std::unique_ptr<fcgi::Request>> requests;
			
			static void thread_func(std::list<fcgi::Connection>::iterator iterator);
			
			class GetValueHandler: private NVP::Deserializer {
				private:
					fcgi::ostream &output_stream;
					void new_key(NVP::string_view_pair pair) override;
				public:
					GetValueHandler(fcgi::ostream &stream) : output_stream(stream) {};
					void handle_packet(std::unique_ptr<ipacket> &packet);
			} get_value_handler;
			fcgi::ostream get_value_result_stream;
		public:
			/** Global list of connections
			 *
			 * This is an iterable list of all allocated connections.
			 * It's synchronized via #global_list_mutex.
			 */
			static std::list<Connection> global_list;
			static std::mutex            global_list_mutex;
			/** Socket descriptor
			 */
			const int fd;
			/** Send datas on the wire
			 */
			void send_packet(const char *buffer, size_t size);
			/** Send a packet with packed header/data
			 * \param buffer A writable pointer to the header
			 * \param contentLength Content length written into buffer->contentLength
			 *
			 * This is a convenience wrapper that take a pointer to a header in a
			 * structure like fcgi::ostreambuf::send_buffer.
			 * Note that only the contentLength field is converted to big-endian.
			 */
			void send_packet(Header *buffer, uint16_t contentLength) {
				buffer->contentLength = htons(contentLength);
				return send_packet(reinterpret_cast<const char*>(buffer),sizeof(buffer)+contentLength);
			}
			
			template <typename T, class... Args>
			void send_packet(fcgi::RecType type, uint16_t requestId, Args&&... args) {
				struct {
					fcgi::Header header;
					T body;
				} packet = {{1,type,htons(requestId),htons(sizeof(T)),0,0},{std::forward<Args>(args)...}};
				send_packet(reinterpret_cast<char*>(&packet),sizeof(packet));
			}
			
			/** Thread-safe close the  FastCGI listening socket
			 */
			static void close_listening_socket(void);
			
			static void create_connection(int fd);
			Connection(int fd);
			Connection(Connection&) = delete;
			~Connection(void);
	};
}
