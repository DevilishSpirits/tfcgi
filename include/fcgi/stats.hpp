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
/** \file stats.hpp
 *  \brief Various stats and performances counters
 *
 * This file expose fcgi::GlobalStats that contain many statistics about current
 * health of the framework.
 *
 * \note Stats are purely informative and since they are read asynchronously,
 *       you will have slight incoherencies in the results. Also sampling method
 *       is not of top quality to avoid performance bottlenecks.
 * \see examples/stats that contain a nice reusable web monitor.
 */
#pragma once
#include <ostream>
#include <atomic>
#include <vector>
namespace fcgi {
	class Connection;
	/** Global statistics 
	 *
	 * This structure contain multiple stats about a connection.
	 */
	struct ConnectionStats {
		/** An ID
		 *
		 * This id uniquely identify a connection.
		 * \note Currently id are reused since this is the socket fd.
		 */
		unsigned int id;
		/** Number of active requests
		 *
			 * This is the number of active requests on the FastCGI protocol. It's
			 * increased when receiving a BEGIN_REQUEST packet and decreased when we
			 * END_REQUEST a packet.
		 */
		unsigned int active_requests;
		/** Socket input is sane
		 *
		 * \see fcgi::Connection::socket_insane
		 */
		unsigned int socket_insane;
		ConnectionStats(Connection &connection);
	};
	/** Global statistics 
	 *
	 * This global structure contain multiple stats about the FastCGI framework.
	 * 
	 * It host many structures for more detailed topics.
	 *
	 * \note You must call update() to initialize the structure and poll stats.
	 */
	struct GlobalStats {
		/** Listening state
		 *
		 * True if we are listening for new connections.
		 */
		bool socket_listening;
		/** Quick shutdown state
		 *
		 * True if we are doing a quick_shutdown.
		 */
		bool quick_shutdown;
		
		/** Connections infos
		 *
		 */
		std::vector<ConnectionStats> connections;
		void update(void);
	};
}
