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
#include "protocol.hpp"
#include <queue>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <istream>
#include <ostream>
#include <memory>
#include <arpa/inet.h>

namespace fcgi {
	class ipacket;
	class istream;
	class Connection;
	class Request;
	
	/** Stream buffer for outgoing data
	 *
	 */
	class ostreambuf: public std::streambuf {
		private:
			friend Request;
			struct {
				Header header;
				char data[4096];
			} send_buffer;
			int sync() override;
			int_type overflow(int_type c) override;
			ostreambuf operator=(ostreambuf&) = delete;
			/** If true, send EOF upon destruction
			 *
			 * This variable is set to true when datas are sent and before ending a
			 * request. The design of ostream that regenerate a fresh ostreambuf on
			 * ostream::close() require that flag to avoid weird EOF packets that came
			 * from nowhere.
			 */
			bool send_eof;
		public:
			ostreambuf(Connection &connection, uint16_t requestId, RecType type);
			~ostreambuf();
			Connection& connection;
			fcgi::RecType pkt_type(void) const {
				return send_buffer.header.type;
			}
			uint16_t requestId(void) const {
				return send_buffer.header.requestId;
			}
	};
	/** Stream buffer for ingoing data
	 *
	 */
	class istreambuf: public std::streambuf {
		private:
			friend istream;
			bool is_open;
			std::queue<std::unique_ptr<std::vector<char>>> queue;
			std::mutex queue_mutex;
			std::condition_variable queue_bell;
			
			std::unique_ptr<std::vector<char>> current_packet;
			
			istreambuf operator=(istreambuf&) = delete;
			int_type underflow(void) override;
			
			void feed(std::unique_ptr<ipacket> &packet);
	};
	
	/** Stream buffer for outgoing data
	 *
	 */
	class ostream: public std::ostream {
		private:
			friend Connection;
			friend Request;
			std::unique_ptr<ostreambuf> internal_streambuf;
			ostream(Connection &connection, uint16_t requestId, RecType type);
			ostream(Request &request, RecType type);
			void reset(void);
		public:
			fcgi::ostream& operator=(fcgi::ostream&& rvalue);
			/* TODO
			inline void keyval_serialize(const std::string_view& key, const std::string_view& val)
			{
				fcgi::NVP::keyval_serialize(*this,key,val);
			}*/
			/** Close the stream
			 */
			void close(void);
	};
	class istream: public std::istream {
		private:
			friend Request;
			std::unique_ptr<istreambuf> internal_streambuf;
			
			void feed(std::unique_ptr<ipacket> &packet) {
				return internal_streambuf->feed(packet);
			}
			void reset(void);
		public:
			istream(void);
			fcgi::istream& operator=(fcgi::istream&& rvalue);
			/*inline void keyval_serialize(const std::string_view& key, const std::string_view& val)
			{
				fcgi::keyval_serialize(*this,key,val);
			}
			inline ostream& operator<<(std::pair<std::string_view,std::string_view> keyval)
			{
				keyval_serialize(keyval.first,keyval.second);
				return *this;
			}*/
	};
}
