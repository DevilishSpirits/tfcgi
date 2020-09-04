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
#include "iostream.hpp"
#include "name-value-pairs.hpp"
#include <memory>
#include <vector>
#include <string_view>
#include <atomic>
namespace fcgi {
	class Connection;
	class ipacket;
	class Request {
		private:
			friend Connection;
			enum State {
				/** Request ready to use
				 *
				 * This state mean that no server request is associated with this one
				 */
				READY,
				/** Reading params
				 *
				 * This state mean that we are receiving datas from the PARAMS stream.
				 */
				READING_PARAMS,
				/** Request is pending
				 *
				 * All worker threads are busy and this request is enqueued.
				 */
				PENDING,
				/** Request running in the application
				 *
				 * The request is being handled by the application.
				 */
				APP_RUNNING,
			};
			std::atomic<State> state;
			BeginRequestBody priv_begin_request;
			
			std::unique_ptr<NVP::ToMap> params_deserializer;
			
			void handle_packet(std::unique_ptr<ipacket> &a_packet);
			void finish(void);
		public:
			const uint16_t requestId;
			Connection &connection;
			const BeginRequestBody& begin_request(void) const {
				return priv_begin_request;
			}
			
			NVP::map params;
			
			istream cin;
			istream cdata;
			ostream cout;
			ostream cerr;
			
			Request(Connection &connection, uint16_t requestId);
			
			// Some utilities
			void sendHeader(const std::string& key, const std::string& value) {
				cout << key << ": " << value << '\n';
			}
			void sendHeader(std::pair<std::string,std::string> &keyval) {
				cout << keyval.first << ": " << keyval.second << '\n';
			}
			void sendStatus(const unsigned int code) {
				cout << "Status: " << code << '\n';
			}
			void sendStatus(const unsigned int code, const std::string &msg) {
				cout << "Status: " << code << ' ' << msg << '\n';
			}
			void sendRedirect(const std::string &destination) {
				cout << "Redirect: " << destination << '\n';
			}
			void sendContent_Type(const std::string &mime) {
				cout << "Content-Type: " << mime << '\n';
			}
			void finishHeaders() {
				cout << '\n';
			}
	};
};
