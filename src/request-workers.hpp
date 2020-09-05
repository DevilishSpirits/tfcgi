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
#include <fcgi/request.hpp>
#include <mutex>
#include <condition_variable>
#include <thread>
namespace fcgi {
	class worker: public std::thread {
		private:
			static void thread_func(void);
		public:
			static void push_request(Request &request);
			
			static std::atomic<unsigned int> workers_online;
			static std::atomic<unsigned int> workers_sleeping;
			static std::atomic<unsigned int> workers_backlog;
			
			worker(void) : std::thread(thread_func) {};
	};
};
