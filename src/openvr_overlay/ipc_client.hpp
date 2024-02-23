#pragma once

#include "../ipc_protocol.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

class IPCClient {
public:
	~IPCClient();

	void Connect();
	protocol::Response SendBlocking(const protocol::Request& request);

	void Send(const protocol::Request& request);
	protocol::Response Receive();

private:
	HANDLE pipe = INVALID_HANDLE_VALUE;
};