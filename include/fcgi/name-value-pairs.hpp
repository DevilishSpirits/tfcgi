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
#include <vector>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
namespace fcgi {
	class ipacket;
	/** FastCGI Name-Value Pairs
	 *
	 * This namespace handle this part of the protocol described in section 3.4 of
	 *  the FastCGI 1.0 specification.
	 */
	namespace NVP {
			typedef std::pair<std::string,std::string> string_pair;
			typedef std::pair<std::string_view,std::string_view> string_view_pair;
			typedef std::unordered_map<std::string,std::string> map;
		/** FastCGI Name-Value pairs deserializer
		 *
		 * \note You should avoid to split keyval accross packets. Doing so remove the
		 *       userspace copy within this implementation and should enhance
		 *       performances.
		 * \warning An instance may suddenly allocate up to 4GB on malicious inputs.
		 */
		class Deserializer {
			private:
				std::vector<char> string_buffer;
				uint32_t name_len;
				uint32_t value_len;
				
				std::string name;
				
				char* current_buffer;
				size_t remaining_len = 0;
				
				/** Parser states
				 */
				enum {
					/** Parser is reading a new pair
					 */
					READ_NEW,
					/** Parser is reading the name_len field
					 */
					READ_NAME_LEN,
					/** Parser is reading the value_len field
					 */
					READ_VALUE_LEN,
					/** Parser will read data
					 */
					WILL_READ_BUFFER,
					/** Parser is reading data
					 */
					READ_BUFFER,
				} parse_state = READ_NEW;
				
				inline void handle_keyval(char *buffer);
				inline bool read_keyval_length(char* &data, uint16_t &len, uint32_t &dest);
			protected:
				/** Keyval decoded
				 *
				 * It's the main function to implement.
				 * \warning Those std::string_view are only valid within the function.
				 *          Perform a copy if you want to keep them around.
				 */
				virtual void new_key(string_view_pair pair) = 0;
			public:
				virtual ~Deserializer() = default;
				/** Feed some datas
				 * \return True if all datas have been read
				 */
				bool feed(std::unique_ptr<ipacket> &packet);
			};
			/** Trivial deserializer to fcgi::Deserializer::map
			 *
			 * \see CreateMap
			 */
			class ToMap: public Deserializer {
				private:
					NVP::map &map;
					void new_key(string_view_pair pair) override {
						map[std::string(pair.first)] = std::string(pair.second);
					}
				public:
					ToMap(NVP::map &map) : map(map) {};
			};
			/** Trivial deserializer to a new fcgi::Deserializer::map
			 *
			 * This deserializer should be wrapped in a std::unique_ptr and the map
			 * retrieved later with sink().
			 * \see CreateMap
			 */
			class CreateMap: public ToMap {
				private:
					NVP::map map;
				public:
					NVP::map& get_map() {
						return map;
					}
					CreateMap(void) : NVP::ToMap(map), map(1) {};
					/** Extract the map from the CreateMap instance and destroy it
					 *
					 * This function use std::swap to avoid wasteful copies
					 */
					static NVP::map sink(std::unique_ptr<CreateMap> deserializer) {
						NVP::map result;
						std::swap(result,deserializer->map);
						deserializer.reset();
						return result;
					}
		};
		
		/** FastCGI Name-Value Pairs serializer
		 * \param key The Name. Cannot exceed 0x8FFFFFFF chars.
		 * \param val The Value. Cannot exceed  0x8FFFFFFF chars.
		 */
		void serialize(std::ostream& target, const std::string_view &key, const std::string_view &val);
	}
}
