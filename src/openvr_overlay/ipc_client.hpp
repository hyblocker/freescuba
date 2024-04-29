#pragma once

#include <openvr.h>
#include "../ipc_protocol.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

class IPCClient {
public:
	~IPCClient();

	void Connect();
	protocol::Response_t SendBlocking(const protocol::Request_t& request) const;

	void Send(const protocol::Request_t& request) const;
	protocol::Response_t Receive() const;

private:
	HANDLE pipe = INVALID_HANDLE_VALUE;
};