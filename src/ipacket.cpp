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
#include "ipacket.hpp"

fcgi::ipacket::ipacket(Connection& connection) : connection(connection)
{
	// Read the packet
	priv_header.contentLength = 0; // FIXME This fix prevent from errors if the subread fail
	connection.recv_all(reinterpret_cast<char*>(&priv_header),sizeof(Header));
	priv_header.contentLength = ntohs(priv_header.contentLength);
}
fcgi::ipacket::~ipacket(void)
{
	char beef[65536];
	if (remaining())
		recv_all(beef,remaining());
}
bool fcgi::ipacket::consume(ssize_t consumed)
{
	// Check overflows
	if (consumed > remaining())
		// FIXME Find a better exception type
		throw std::logic_error("Consumed " + std::to_string(consumed) + " bytes from a FastCGI packet of " + std::to_string(remaining()) + " bytes. Protocol is broken."); 
	// Check consumed
	if (consumed < 0)
		switch (errno) {
			case EINTR:
			case EAGAIN:
				return true;
			default:
				throw std::logic_error("Consumed " + std::to_string(consumed) + " bytes from a FastCGI packet (yes, that's a negative amount). Probably a socket error.");
	} else if (consumed == 0)
		throw std::logic_error("Consumed 0 bytes from a FastCGI packet. The socket is probably closed.");
	priv_header.contentLength -= consumed;
	// Skip padding if any 
	if (!priv_header.contentLength && priv_header.paddingLength) {
		char trash[255];
		connection.recv_all(trash,priv_header.paddingLength);
	}
	return false;
}
void fcgi::ipacket::recv_all(char *data, size_t size)
{
	// Check overflows
	if (size > remaining())
		throw std::logic_error("Tried to read " + std::to_string(size) + " bytes from a FastCGI packet with " + std::to_string(remaining()) + " bytes remaining.");
	if (size == 0)
		return; // Do nothing
	// Read packet
	connection.recv_all(data,size);
	consume(size);
}
std::vector<char> fcgi::ipacket::sink_to_vector(void)
{
	std::vector<char> result;
	result.resize(remaining());
	recv_all(result.data(),remaining());
	return result;
}
