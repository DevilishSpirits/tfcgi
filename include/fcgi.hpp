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
#include "fcgi/request.hpp"
#include "fcgi/stats.hpp"

#if __cplusplus < 201703L
	#warning You compiler does not seem to support C++17
#endif

#if !(FCGI_RESPONDER || FCGI_AUTHORIZER || FCGI_FILTER || DFCGI_CUSTOM_ROLES)
	#warning No FastCGI role enabled. It is probably not what you want, see TUTORIAL.md
#endif

namespace fcgi {
	/** Initialization callback
	 * \return Zero on success, else it's the program exit-code
	 *
	 * This callback perform process-wide initialization.
	 */
	int startup(int argc, char** argv);
	#if FCGI_RESPONDER
	/** Handle responder request
	 * \param request The request to handle
	 * \return The status code
	 *
	 * You handle requests in this callback.
	 *
	 * This call is guarded by a catch-all try block. If an unhandled exception
	 * occur, details are send in request.cerr (if not closed) and the syslog.
	 * On some errors, the framework may return the `FCGI_OVERLOADED` protocol
	 * status like when you run out of some resources (files, memory, ...).
	 */
	int respond(Request &request);
	#endif
	#if FCGI_AUTHORIZER
	// TODO
	/* Handle authorizer request
	 * \param request The request to handle
	 * \return The status code
	 *
	 * This handle requests in this callback.
	 *
	 * This call is guarded by a catch-all try block. If an unhandled exception
	 * occur, details are send in request.cerr (if not closed) and the syslog.
	 * On some errors, the framework may return the `FCGI_OVERLOADED` protocol
	 * status like when you run out of some resources (files, memory, ...).
	 */
	//int authorize(Request &request);
	#endif
	#if FCGI_CUSTOM_ROLES
	/** Unknow request handling callback
	 * \param request The request to handle
	 * \param result  The content of the END_REQUEST packet. It's initialized
	 *                    to an unknow-role reply with the role id.
	 *
	 * In case a role is not handled by the framework, you can ultimately use this
	 * callback to handle it.
	 *
	 * This call is guarded by a catch-all try block. If an unhandled exception
	 * occur, details are send in request.cerr (if not closed) and the syslog.
	 * On some errors, the framework may return the `FCGI_OVERLOADED` protocol
	 * status like when you run out of some resource (files, memory, ...).
	 *
	 * If you let result with the unknow role protocol status, a message will be
	 * sent to cerr
	 */
	void custom(Request &request, EndRequestBody &result);
	#endif
	/** Early shutdown callback
	 * \param quick True if fast-termination is desired (SIGTERM, ...)
	 * 
	 * This callback is called when the FastCGI server is scheduling a shutdown.
	 * All requests are still running in parallel.
	 * 
	 * You may notify requests of that and pre-release things (beware that new
	 * request can be accepted even after that).
	 */
	void begin_shutdown(bool quick);
	/** Shutdown callback
	 * \param quick True if fast-termination is desired (SIGTERM, ...)
	 * \return Process exit-code
	 *
	 * This callback deinit what have been initialized in fcgi::startup. It's
	 * called when all connections are closed and is hence trhead-safe.
	 */
	int shutdown(bool quick);
}
