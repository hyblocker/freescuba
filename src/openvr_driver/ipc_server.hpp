#pragma once

#include "openvr_driver.h"

#include <thread>
#include <mutex>
#include <set>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "../ipc_protocol.hpp"

class DeviceProvider;

class IPCServer {
public:
	IPCServer(DeviceProvider* driver) : driver(driver), connectEvent(INVALID_HANDLE_VALUE){ }
	~IPCServer();

	void Run();
	void Stop();

private:

	void HandleRequest(const protocol::Request& request, protocol::Response& response);

	struct PipeInstance
	{
		OVERLAPPED overlap; // Used by the API
		HANDLE pipe;
		IPCServer* server;

		protocol::Request request;
		protocol::Response response;
	};

	PipeInstance* CreatePipeInstance(HANDLE pipe);
	void ClosePipeInstance(PipeInstance* pipeInst);

	static void RunThread(IPCServer* _this);
	static BOOL CreateAndConnectInstance(LPOVERLAPPED overlap, HANDLE& pipe);
	static void WINAPI CompletedReadCallback(DWORD err, DWORD bytesRead, LPOVERLAPPED overlap);
	static void WINAPI CompletedWriteCallback(DWORD err, DWORD bytesWritten, LPOVERLAPPED overlap);

	std::thread mainThread;

	bool running = false;
	bool stop = false;

	std::set<PipeInstance*> pipes;
	HANDLE connectEvent;

	DeviceProvider* driver;
};