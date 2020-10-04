#pragma once
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
namespace fcgi {
	/** Limits of the framework
	 *
	 * This namespace bundle all limits of the framework.
	 */
	namespace limits {
		/** Maximum size of a Name-Value Pair for fcgi::NVP::Deserializer
		 *
		 * A Name-Value Pair can size up to 4GiB, the implementation may allocate a
		 * buffer of this size so we must have a limit.
		 *
		 * The implementation throw a fcgi::NVP::Deserializer::pair_size_error when
		 * the limit is exceeded.
		 *
		 * This limit the size of the name and value combined of one NVP. It's does
		 * not limit the amount of memory consumed by all NVPs.
		 * \see Per instance limit override fcgi::NVP::Deserializer::max_pair_size.
		 * \note When this limit is exceeded by a request, the server will return an
		 *       empty response, a message in cerr with code.
		 */
		extern size_t nvp_max_pair_size;
	}
}
