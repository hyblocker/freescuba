/*

#pragma once

#include "openvr_driver.h"
#include "../ipc_protocol.hpp"

#include <thread>
#include <set>
#include <mutex>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

class DeviceProvider;

class IPCServer {
public:
	IPCServer(DeviceProvider* driver) : driver(driver) {}
	~IPCServer();

	void Run();
	void Stop();

private:

	void HandleRequest(const protocol::Request& request, protocol::Response& response);
	
	#define PIPE_BUFFER_SIZE 4096

	struct PipeBuffer {
		uint8_t buffer[PIPE_BUFFER_SIZE]; // output buffer
		uint32_t bufferSize;
	};

	struct PipeInstance
	{
		OVERLAPPED overlap; // Used by the API
		HANDLE pipe;
		IPCServer* server;

		PipeBuffer readBuffer;
		PipeBuffer writeBuffer;

		// protocol::Request request; // output
		// protocol::Response response; // input
	};

	PipeInstance* CreatePipeInstance(HANDLE pipe);
	void ClosePipeInstance(PipeInstance* pipeInst);

	static void RunThread(IPCServer* _this);
	static BOOL CreateAndConnectInstance(LPOVERLAPPED overlap, HANDLE& pipe);
	static BOOL IPCServer::ConnectToNewClient(HANDLE hPipe, LPOVERLAPPED overlap);
	static void WINAPI CompletedReadCallback(DWORD err, DWORD bytesRead, LPOVERLAPPED overlap);
	static void WINAPI CompletedWriteCallback(DWORD err, DWORD bytesWritten, LPOVERLAPPED overlap);

	std::thread mainThread;

	bool running = false;
	bool stop = false;

	std::set<PipeInstance*> pipes;
	HANDLE connectEvent;

	DeviceProvider* driver;

};

*/

#pragma once

#include <openvr_driver.h>
#include "../ipc_protocol.hpp"

#include <thread>
#include <set>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

class DeviceProvider;

namespace Hekky {
	namespace IPC {

#define PIPE_TIMEOUT 5000
#define PIPE_BUFFER_SIZE 4096

		struct PipeBuffer {
			uint8_t data[PIPE_BUFFER_SIZE];
			uint64_t dataSize;
		};

		class IPCServer {

			struct PipeInstance {
				OVERLAPPED oOverlap;
				HANDLE hPipeInst;
				IPCServer* server;

				union {
					PipeBuffer readBuffer;
					protocol::Request request;
				};
				union {
					PipeBuffer writeBuffer;
					protocol::Response response;
				};
			};

		public:
			IPCServer(DeviceProvider* driver) : m_driver(driver), m_connectEvent(INVALID_HANDLE_VALUE) {} // : driver(driver) {}
			~IPCServer();

			void Run();
			void Stop();

		private:
			void HandleRequest(PipeInstance* pipe); // pipeInst->server->HandleRequest(pipeInst->request, pipeInst->response);
			// void HandleRequest(const protocol::Request& request, protocol::Response& response);

			PipeInstance* CreatePipeInstance(HANDLE pipe);
			void ClosePipeInstance(PipeInstance* pipeInst);

			static void RunThread(IPCServer* _this);

			static BOOL CreateAndConnectInstance(LPOVERLAPPED lpoOverlap, HANDLE& pipe);
			static BOOL ConnectToNewClient(HANDLE hPipe, LPOVERLAPPED lpo);

			static VOID WINAPI CompletedWriteRoutine(DWORD dwErr, DWORD cbWritten, LPOVERLAPPED lpOverLap);
			static VOID WINAPI CompletedReadRoutine(DWORD dwErr, DWORD cbBytesRead, LPOVERLAPPED lpOverLap);

		private:

			std::thread m_pipeThread;

			bool running = false;
			bool stop = false;

			std::set<PipeInstance*> pipes;
			HANDLE m_connectEvent;

			DeviceProvider* m_driver;
		};
	}
}