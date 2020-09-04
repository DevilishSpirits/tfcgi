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
#include <cstdint>
// This is a C++ version of https://web.archive.org/web/20160119141816/http://www.fastcgi.com/drupal/node/6?q=node%2F22#S8

namespace fcgi {
	
	constexpr const auto listening_sock_fileno = 0;
	
	enum RecType : uint8_t {
		BEGIN_REQUEST     = 1,
		ABORT_REQUEST     = 2,
		END_REQUEST       = 3,
		PARAMS            = 4,
		STDIN             = 5,
		STDOUT            = 6,
		STDERR            = 7,
		DATA              = 8,
		GET_VALUES        = 9,
		GET_VALUES_RESULT = 10,
		UNKNOWN_TYPE      = 11,
	};
	
	struct Header {
		uint8_t  version;
		RecType  type;
		uint16_t requestId;
		uint16_t contentLength;
		uint8_t paddingLength;
		uint8_t reserved;
	};
	
	enum BeginRequestRole: uint16_t {
		RESPONDER  = 1,
		AUTHORIZER = 2,
		FILTER     = 3,
	};
	struct BeginRequestBody {
		BeginRequestRole role;
		uint8_t flags;
		uint8_t reserved[5];
	};
	/*
	 * Mask for flags component of FCGI_BeginRequestBody
	 */
	#define FCGI_KEEP_CONN  1
	
	enum EndRequestProtoStatus: uint8_t {
		REQUEST_COMPLETE = 0,
		CANT_MPX_CONN    = 1,
		OVERLOADED       = 2,
		UNKNOWN_ROLE     = 3,
	};
	struct EndRequestBody {
		uint32_t appStatus;
		EndRequestProtoStatus protocolStatus;
		uint8_t reserved[3];
	};
	struct UnknownTypeBody {
		uint8_t type;
		uint8_t reserved[7];
	};
}
