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
#include <fcgi/protocol.hpp>
#include <vector>
#include "connection.hpp"
namespace fcgi {
	class Connection;
	/** Incoming packet informations
	 *
	 * This is an incoming packet representation, you are expected to consume()
	 * it's content by reading the fd()/
	 *
	 * \warning No more data can be readed until this object is destroyed
	 */
	class ipacket {
		private:
			/** The connection of the ipacket
			 */
			Connection& connection;
			/** The header of the ipacket
			 */
			Header priv_header;
		public:
			/** Read a packet from the given connection
			 *
			 */
			ipacket(Connection& connection);
			
			~ipacket(void);
			/** Socket fd
			 * 
			 * You have to perform operations from this file descriptor.
			 */
			int fd(void) const {
				return connection.fd;
			}
			/** Get the header of the ipacket
			 *
			 * Header::contentLength reduce after calls to consume() as it contain
			 * the remaining packet size
			 */
			const Header& header(void) const {
				return priv_header;
			}
			/** Get the remaining amount of data
			 *
			 * Try to read more than this amount of datas is an error
			 */
			uint16_t remaining(void) const {
				return priv_header.contentLength;
			};
			/** Notify that datas have been consumed
			 * \param consumed The number of bytes read. It's the result of the
			 *                 read() or simmilar function.
			 *                 Passing zero will throw an exception about unexpected
			 *                 end of the protocol.
			 * \return Wheather you **must** try again the call. It's true upon
			 *         EINTR or EAGAIN
			 *
			 * You must call after extraction some bytes. It automatically perform
			 * error checking and will throw if something is wrong.
			 */
			bool consume(ssize_t consumed);
			/** Read data in memory
			 *
			 * This is a convenience function that mimic read(3)
			 */
			void recv_all(char *data, size_t size);
			/** Sink all remaining data into a newly allocated vector
			 */
			std::vector<char> sink_to_vector(void);
			/** Sink all remaining data into a newly allocated vector
			 * \param packet[in] A pointer to the packet
			 *
			 * This version consume a std::unique_ptr for conveninece.
			 */
			static std::vector<char> sink_to_vector(std::unique_ptr<ipacket> packet) {
				std::unique_ptr<ipacket> local_packet = std::move(packet);
				return local_packet->sink_to_vector();
			}
	};
}
