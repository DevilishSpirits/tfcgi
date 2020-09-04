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
#include <fcgi/iostream.hpp>
#include <fcgi/request.hpp>
#include "ipacket.hpp"

fcgi::ostreambuf::ostreambuf(Connection &connection, uint16_t requestId, RecType type) : send_eof(false), connection(connection)
{
	// Prefill the header
	send_buffer.header.version = 1;
	send_buffer.header.type = type;
	send_buffer.header.requestId = htons(requestId);
	send_buffer.header.paddingLength = 0;
	send_buffer.header.reserved = 0;
	// Set the buffer
	setp(send_buffer.data,&send_buffer.data[sizeof(send_buffer.data)]);
}
fcgi::ostreambuf::~ostreambuf()
{
	// Flush
	sync();
	// Send the EOF packet
	if (send_eof)
		connection.send_packet(&send_buffer.header,0);
}
int fcgi::ostreambuf::sync()
{
	// Compute contentLength 
	const auto contentLength = reinterpret_cast<uintptr_t>(pptr()) - reinterpret_cast<uintptr_t>(pbase());
	if (contentLength) {
		// Only send packets with datas
		pbump(-contentLength);
		connection.send_packet(&send_buffer.header,contentLength);
		send_eof = true;
	}
	return 0;
}
fcgi::ostreambuf::int_type fcgi::ostreambuf::overflow(fcgi::ostreambuf::int_type c)
{
	if (sync() != 0)
		return traits_type::eof();
	if (c != traits_type::eof()) {
		pbase()[0] = c;
		pbump(1);
	}
	return c;
}
fcgi::ostream::ostream(Request &request, RecType type) : ostream(request.connection,request.requestId,type)
{
}
fcgi::ostream::ostream(Connection &connection, uint16_t requestId, RecType type) : internal_streambuf(std::make_unique<fcgi::ostreambuf>(connection,requestId,type))
{
	rdbuf(internal_streambuf.get());
}
fcgi::ostream& fcgi::ostream::operator=(fcgi::ostream&& rvalue)
{
	// Steal the internal buffer
	internal_streambuf = std::move(rvalue.internal_streambuf);
	rdbuf(rvalue.rdbuf(NULL));
	return *this;
}
void fcgi::ostream::close(void)
{
	if (rdbuf()) {
		flush();
		// I clear the rdbuf but I preallocate a new ostreambuf
		rdbuf(NULL);
		internal_streambuf = std::make_unique<fcgi::ostreambuf>(internal_streambuf->connection,internal_streambuf->requestId(),internal_streambuf->pkt_type());
	}
};
void fcgi::ostream::reset()
{
	// I already have a ready to use instance of internal_streambuf
	rdbuf(internal_streambuf.get());
}

fcgi::istreambuf::int_type fcgi::istreambuf::underflow(void)
{
	std::unique_lock<std::mutex> lock(queue_mutex);
	// Wait for next one
	while (queue.empty() && is_open)
		queue_bell.wait(lock);
	// Handle messages
	if (is_open) {
		// Pop the queue
		current_packet = std::move(queue.front());
		queue.pop();
		
		if (current_packet->empty()) {
			// Reached end-of-stream
			is_open = false;
			queue_bell.notify_all(); // Unlock all threads
		} else {
			// Replace the buffer
			char *begin_of_data = &(current_packet->operator[](0));
			setg(begin_of_data,begin_of_data,&(begin_of_data[current_packet->size()]));
			return begin_of_data[0];
		}
	}
	// Here (is_open == false)
	return EOF;
}
void fcgi::istreambuf::feed(std::unique_ptr<fcgi::ipacket> &packet)
{
	std::unique_lock<std::mutex> lock(queue_mutex);
	queue.push(std::make_unique<std::vector<char>>(packet->sink_to_vector()));
	packet.reset();
}
fcgi::istream::istream(void)
{
	reset();
}
fcgi::istream& fcgi::istream::operator=(fcgi::istream&& rvalue)
{
	// Steal the internal buffer
	internal_streambuf = std::move(rvalue.internal_streambuf);
	rdbuf(rvalue.rdbuf(NULL));
	return *this;
}
void fcgi::istream::reset(void)
{
	internal_streambuf = std::make_unique<fcgi::istreambuf>();
}
