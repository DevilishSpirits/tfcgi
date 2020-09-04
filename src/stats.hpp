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
#include <fcgi/stats.hpp>
#include <atomic>
namespace fcgi {
	/** Internal statistics structure
	 *
	 * This global store various states in the programs. It's not just statistics.
	 */
	extern struct InternalStats {
			/** Socket listening authoritative status
			 *
			 * It's true when we are accepting new connections.
			 * When false, the main thread is waiting for workers that will shutdown
			 * if socket_count is also zero.
			 */
			std::atomic<bool> socket_listening;
			/** Quick-shutdown
			 *
			 * If true we are doing a quick shutdown. #socket_listening is also false.
			 */
			std::atomic<bool> quick_shutdown = false;
			/** Number of connections
			 *
			 * It's true when we are accepting new connections.
			 * When false, the main thread is waiting for workers that will shutdown
			 * if there's no active connection.
			 */
			std::atomic<unsigned int> socket_active = 0;
	} internal_stats;
}
