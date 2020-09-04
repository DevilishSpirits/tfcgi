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
#include <fcgi/name-value-pairs.hpp>
#include "ipacket.hpp"
#include <cstring>
#include <arpa/inet.h>
#include <stdexcept>

/** We have a keyval pair ready for use
 */
inline void fcgi::NVP::Deserializer::handle_keyval(char *buffer)
{
	// char *buffer layout is
	new_key( // struct { char key[name_len]; char value[value_len]; };
		std::make_pair(
			std::string_view(buffer,name_len),
			std::string_view(&buffer[name_len],value_len) 
		)
	);
}
#define forward(amount) { \
	auto macro_amount = (amount); \
	data += macro_amount; \
	len -= macro_amount; \
	current_buffer += macro_amount; \
}
inline bool fcgi::NVP::Deserializer::read_keyval_length(char* &data, uint16_t &len, uint32_t &dest)
{
	if (data[0] & 0x80) {
		if (len < sizeof(uint32_t)) {
			// Damn !
			memcpy(&dest,data,len);
			remaining_len = sizeof(uint32_t) - len;
			current_buffer = reinterpret_cast<char*>(&dest) + len;
			reinterpret_cast<char*>(&dest)[0] &= ~0x80; // Clear the bit
			return true;
		} else {
			// Okay, I just do my job
			memcpy(&dest,data,sizeof(uint32_t));
			forward(sizeof(uint32_t));
			if (len == sizeof(uint32_t))
				return false;
			else return true;
		}
	} else {
		dest = 0;
		reinterpret_cast<uint8_t*>(&dest)[3] = data[0];
		forward(1);
		return false;
	}
}

bool fcgi::NVP::Deserializer::feed(std::unique_ptr<ipacket> &packet)
{
	std::vector<char> datas = packet->sink_to_vector();
	packet.reset();
	char* data = datas.data();
	uint16_t  len = datas.size();
	if (len == 0) {
		// TODO Reset parser
		return true;
	}
	// Finish the current state
	if (remaining_len > len) {
		memcpy(current_buffer,data,len);
		remaining_len -= len;
		return false;
	} else {
		memcpy(current_buffer,data,remaining_len);
		forward(remaining_len);
	}
	// Main parse loop
	do {
		switch (parse_state) {
			// Read lengths
			case NVP::Deserializer::READ_NEW:
				// Begin to read name_len
				// Here len > 0
				if (read_keyval_length(data,len,name_len)) {
					parse_state = NVP::Deserializer::READ_NAME_LEN;
					return false;
				}
			case NVP::Deserializer::READ_NAME_LEN:
				// Begin to read value_len
				if (read_keyval_length(data,len,value_len)) {
					parse_state = NVP::Deserializer::READ_VALUE_LEN;
					return false;
				}
			case NVP::Deserializer::READ_VALUE_LEN: // FIXME Is that here ?
			case NVP::Deserializer::WILL_READ_BUFFER:
				name_len  = ntohl(name_len);
				value_len = ntohl(value_len);
				if (len >= name_len + value_len) {
					// 😀️ Zero userspace copy !
					handle_keyval(data);
					parse_state = NVP::Deserializer::READ_NEW;
					forward(name_len + value_len);
					continue;
				} else {
					// 😭️ Split packets
					remaining_len = name_len + value_len;
					string_buffer.resize(remaining_len);
					current_buffer = string_buffer.data();
					memcpy(current_buffer,data,len);
					remaining_len -= len;
					current_buffer += len;
					return false;
				}
			case NVP::Deserializer::READ_BUFFER:
				// We finally got this
				handle_keyval(string_buffer.data());
				parse_state = NVP::Deserializer::READ_NEW;
		}
	} while (len);
	return false;
}

static void serialize_size(std::ostream& target, size_t size)
{
	if (size > 0x8FFFFFFF)
		throw std::length_error("Name or value of a FastCGI Name-Value Pair exceed 0x8FFFFFFF chars.");
	else if (size > 127) {
		uint32_t length = htonl(size);
		reinterpret_cast<uint8_t*>(&length)[0] |= 0x80;
		target.write(reinterpret_cast<char*>(&length),sizeof(length));
	} else target.put(size);
}
template <typename T>
inline void serialize_template(std::ostream& target, const T &key, const T &val)
{
	serialize_size(target,key.length());
	serialize_size(target,val.length());
	target.write(key.data(),key.length());
	target.write(val.data(),val.length());
}

void fcgi::NVP::serialize(std::ostream& target, const std::string_view &key, const std::string_view &val)
{
	return serialize_template<std::string_view>(target,key,val);
}
