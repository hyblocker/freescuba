#include "ipc_server.hpp"
#include "device_provider.hpp"

namespace Hekky {
	namespace IPC {

		void IPCServer::HandleRequest(PipeInstance* pipe) {
			switch (pipe->request.type)
			{
			case protocol::RequestHandshake:
				pipe->response.type = protocol::ResponseHandshake;
				pipe->response.protocol.version = protocol::Version;
				break;
			case protocol::RequestDevicePose:
				pipe->response.type = protocol::ResponseDevicePose;
				pipe->response.driverPose = m_driver->GetCachedPose(pipe->request.driverPoseIndex);
				break;
			case protocol::RequestUpdateGloveLeftState:
				m_driver->HandleGloveUpdate(pipe->request.gloveData, true);
				pipe->response.type = protocol::ResponseSuccess;
				break;

			case protocol::RequestUpdateGloveRightState:
				m_driver->HandleGloveUpdate(pipe->request.gloveData, false);
				pipe->response.type = protocol::ResponseSuccess;
				break;

			default:
				LOG("Invalid IPC request: %d", pipe->request.type);
				pipe->response.type = protocol::ResponseInvalid;
				break;
			}
			pipe->writeBuffer.dataSize = sizeof(protocol::Response);
		}

		IPCServer::~IPCServer() {
			Stop();
		}

		void IPCServer::Run() {

			m_pipeThread = std::thread(RunThread, this);
		}

		void IPCServer::Stop() {
			LOG("IPCServer::Stop()");
			if (!running)
				return;

			stop = true;
			SetEvent(m_connectEvent);
			m_pipeThread.join();
			running = false;
			LOG("IPCServer::Stop() finished");
		}

		IPCServer::PipeInstance* IPCServer::CreatePipeInstance(HANDLE pipe) {
			PipeInstance* pipeInst = (PipeInstance*)GlobalAlloc(GPTR, sizeof(PipeInstance));
			if (pipeInst == NULL) {
				LOG("GlobalAlloc failed (%d)", GetLastError());
				return nullptr;
			}

			// auto pipeInst = new PipeInstance;
			pipeInst->hPipeInst = pipe;
			pipeInst->server = this;

			// 0 out the buffers
			memset(pipeInst->readBuffer.data, 0, sizeof pipeInst->readBuffer.data);
			pipeInst->readBuffer.dataSize = 0;

			memset(pipeInst->writeBuffer.data, 0, sizeof pipeInst->writeBuffer.data);
			pipeInst->writeBuffer.dataSize = 0;

			pipes.insert(pipeInst);
			return pipeInst;
		}

		void IPCServer::ClosePipeInstance(PipeInstance* pipeInst) {
			if (!DisconnectNamedPipe(pipeInst->hPipeInst)) {
				LOG("DisconnectNamedPipe failed with %d.", GetLastError());
			}
			CloseHandle(pipeInst->hPipeInst);
			pipes.erase(pipeInst);
			// delete pipeInst;

			if (pipeInst != NULL) {
				GlobalFree(pipeInst);
			}
		}

		void IPCServer::RunThread(IPCServer* _this) {
			_this->running = true;

			OVERLAPPED oConnect;
			PipeInstance* lpPipeInst;
			DWORD dwWait, cbRet;
			BOOL fSuccess, fPendingIO;
			HANDLE hPipe;

			// Create one event object for the connect operation. 
			_this->m_connectEvent = CreateEvent(
				NULL,    // default security attribute
				TRUE,    // manual reset event 
				TRUE,    // initial state = signaled 
				NULL);   // unnamed event object 

			if (_this->m_connectEvent == NULL) {
				LOG("CreateEvent failed with %d.", GetLastError());
				return;
			}

			oConnect.hEvent = _this->m_connectEvent;

			// Call a subroutine to create one instance, and wait for the client to connect. 
			fPendingIO = CreateAndConnectInstance(&oConnect, hPipe);

			while (!_this->stop)
			{
				// Wait for a client to connect, or for a read or write operation to be completed, which causes a completion routine to be queued for execution. 
				dwWait = WaitForSingleObjectEx(
					_this->m_connectEvent,  // event object to wait for 
					INFINITE,       // waits indefinitely 
					TRUE);          // alertable wait enabled 

				if (_this->stop) {
					break;
				}

				switch (dwWait)
				{
					// The wait conditions are satisfied by a completed connect operation. 
				case 0:
				{
					// If an operation is pending, get the result of the connect operation. 
					if (fPendingIO) {
						fSuccess = GetOverlappedResult(
							hPipe,     // pipe handle 
							&oConnect, // OVERLAPPED structure 
							&cbRet,    // bytes transferred 
							FALSE);    // does not wait 

						if (!fSuccess) {
							LOG("ConnectNamedPipe (%d)", GetLastError());
							return;
						}
					}

					LOG("IPC client connected");

					// Allocate storage for this instance.
					lpPipeInst = _this->CreatePipeInstance(hPipe);
					if (lpPipeInst == nullptr) {
						return;
					}

					// Start the read operation for this client. Note that this same routine is later used as a completion routine after a write operation. 
					lpPipeInst->writeBuffer.dataSize = 0;
					CompletedWriteRoutine(0, 0, (LPOVERLAPPED)lpPipeInst);

					// Create new pipe instance for the next client. 
					fPendingIO = CreateAndConnectInstance(&oConnect, hPipe);
					break;
				}

				// The wait is satisfied by a completed read or write operation. This allows the system to execute the completion routine. 

				case WAIT_IO_COMPLETION:
					break;

					// An error occurred in the wait function. 
				default:
				{
					LOG("WaitForSingleObjectEx (%d)", GetLastError());
					return;
				}
				}
			}

			for (auto& pipeInst : _this->pipes) {
				_this->ClosePipeInstance(pipeInst);
			}
			_this->pipes.clear();
		}

		// This function creates a pipe instance and connects to the client. It returns TRUE if the connect operation is pending, and FALSE if the connection has been completed. 
		BOOL IPCServer::CreateAndConnectInstance(LPOVERLAPPED lpoOverlap, HANDLE& pipe) {
			LPCWSTR lpszPipename = TEXT(FREESCUBA_PIPE_NAME);

			pipe = CreateNamedPipe(
				lpszPipename,             // pipe name 
				PIPE_ACCESS_DUPLEX |      // read/write access 
				FILE_FLAG_OVERLAPPED,     // overlapped mode 
				PIPE_TYPE_MESSAGE |       // message-type pipe 
				PIPE_READMODE_MESSAGE |   // message read mode 
				PIPE_WAIT,                // blocking mode 
				PIPE_UNLIMITED_INSTANCES, // unlimited instances 
				PIPE_BUFFER_SIZE * sizeof(CHAR),    // output buffer size 
				PIPE_BUFFER_SIZE * sizeof(CHAR),    // input buffer size 
				PIPE_TIMEOUT,             // client time-out 
				NULL);                    // default security attributes

			if (pipe == INVALID_HANDLE_VALUE) {
				LOG("CreateNamedPipe failed with %d.", GetLastError());
				return 0;
			}

			// Call a subroutine to connect to the new client. 
			return ConnectToNewClient(pipe, lpoOverlap);
		}

		// This routine is called as a completion routine after writing to the pipe, or when a new client has connected to a pipe instance. It starts another read operation.
		VOID WINAPI IPCServer::CompletedWriteRoutine(DWORD dwErr, DWORD cbWritten, LPOVERLAPPED lpOverLap) {
			PipeInstance* lpPipeInst;
			BOOL fRead = FALSE;

			// lpOverlap points to storage for this instance. 
			lpPipeInst = (PipeInstance*)lpOverLap;

			// The write operation has finished, so read the next request (if there is no error). 
			if ((dwErr == 0) && (cbWritten == lpPipeInst->writeBuffer.dataSize)) {
				fRead = ReadFileEx(
					lpPipeInst->hPipeInst,
					lpPipeInst->readBuffer.data,
					PIPE_BUFFER_SIZE * sizeof(CHAR),
					(LPOVERLAPPED)lpPipeInst,
					(LPOVERLAPPED_COMPLETION_ROUTINE)CompletedReadRoutine);
			}

			// Disconnect if an error occurred. 
			if (!fRead) {
				lpPipeInst->server->ClosePipeInstance(lpPipeInst);
			}
		}

		// This routine is called as an I/O completion routine after reading a request from the client. It gets data and writes it to the pipe. 
		VOID WINAPI IPCServer::CompletedReadRoutine(DWORD dwErr, DWORD cbBytesRead, LPOVERLAPPED lpOverLap) {
			PipeInstance* lpPipeInst;
			BOOL fWrite = FALSE;

			// lpOverlap points to storage for this instance. 
			lpPipeInst = (PipeInstance*)lpOverLap;

			// The read operation has finished, so write a response (if no error occurred). 
			if ((dwErr == 0) && (cbBytesRead != 0)) {
				// HandleRequest(lpPipeInst);
				lpPipeInst->server->HandleRequest(lpPipeInst);

				fWrite = WriteFileEx(
					lpPipeInst->hPipeInst,
					lpPipeInst->writeBuffer.data,
					lpPipeInst->writeBuffer.dataSize,
					(LPOVERLAPPED)lpPipeInst,
					(LPOVERLAPPED_COMPLETION_ROUTINE)CompletedWriteRoutine);
			}

			// Disconnect if an error occurred. 
			if (!fWrite) {
				lpPipeInst->server->ClosePipeInstance(lpPipeInst);
			}
		}

		BOOL IPCServer::ConnectToNewClient(HANDLE hPipe, LPOVERLAPPED lpo) {
			BOOL fConnected, fPendingIO = FALSE;

			// Start an overlapped connection for this pipe instance. 
			fConnected = ConnectNamedPipe(hPipe, lpo);

			// Overlapped ConnectNamedPipe should return zero. 
			if (fConnected) {
				LOG("ConnectNamedPipe failed with %d.", GetLastError());
				return 0;
			}

			switch (GetLastError()) {
				// The overlapped connection in progress. 
			case ERROR_IO_PENDING:
				fPendingIO = TRUE;
				break;

				// Client is already connected, so signal an event. 
			case ERROR_PIPE_CONNECTED:
				if (SetEvent(lpo->hEvent))
					break;

				// If an error occurs during the connect operation... 
			default:
			{
				LOG("ConnectNamedPipe failed with %d.", GetLastError());
				return 0;
			}
			}
			return fPendingIO;
		}
	}
}

#if 0
void IPCServer::HandleRequest(const protocol::Request& request, protocol::Response& response)
{
	switch (request.type)
	{
	case protocol::RequestHandshake:
		response.type = protocol::ResponseHandshake;
		response.protocol.version = protocol::Version;
		break;

	case protocol::RequestUpdateFingers:
		driver->HandleGloveUpdate(request.gloveData);
		response.type = protocol::ResponseSuccess;
		break;

	case protocol::RequestUpdateInput:
		driver->HandleGloveUpdate(request.gloveData);
		response.type = protocol::ResponseSuccess;
		break;

	case protocol::RequestUpdateGloveState:
		driver->HandleGloveUpdate(request.gloveData);
		response.type = protocol::ResponseSuccess;
		break;

	default:
		LOG("Invalid IPC request: %d", request.type);
		response.type = protocol::ResponseInvalid;
		break;
	}
}

IPCServer::~IPCServer()
{
	Stop();
}

void IPCServer::Run()
{
	mainThread = std::thread(RunThread, this);
}

void IPCServer::Stop()
{
	TRACE("IPCServer::Stop()");
	if (!running)
		return;

	stop = true;
	SetEvent(connectEvent);
	mainThread.join();
	running = false;
	TRACE("IPCServer::Stop() finished");
}

IPCServer::PipeInstance* IPCServer::CreatePipeInstance(HANDLE pipe) {
	PipeInstance* pipeInst = (PipeInstance*)GlobalAlloc(GPTR, sizeof(PipeInstance));
	if (pipeInst == NULL) {
		printf("GlobalAlloc failed (%d)\n", GetLastError());
		return nullptr;
	}

	// auto pipeInst = new PipeInstance;
	pipeInst->pipe = pipe;
	pipeInst->server = this;

	// 0 out the buffers
	memset(pipeInst->readBuffer.buffer, 0, sizeof pipeInst->readBuffer.buffer);
	pipeInst->readBuffer.bufferSize = 0;

	memset(pipeInst->writeBuffer.buffer, 0, sizeof pipeInst->writeBuffer.buffer);
	pipeInst->writeBuffer.bufferSize = 0;

	pipes.insert(pipeInst);
	return pipeInst;
}

void IPCServer::ClosePipeInstance(PipeInstance* pipeInst) {
	if (!DisconnectNamedPipe(pipeInst->pipe)) {
		printf("DisconnectNamedPipe failed with %d.", GetLastError());
	}
	CloseHandle(pipeInst->pipe);
	pipes.erase(pipeInst);
	// delete pipeInst;

	if (pipeInst != NULL) {
		GlobalFree(pipeInst);
	}
}

/*
IPCServer::PipeInstance* IPCServer::CreatePipeInstance(HANDLE pipe)
{
	auto pipeInst = new PipeInstance;
	pipeInst->pipe = pipe;
	pipeInst->server = this;

	// 0 out the buffers
	memset(pipeInst->readBuffer.buffer, 0, sizeof pipeInst->readBuffer.buffer);
	pipeInst->readBuffer.bufferSize = 0;

	memset(pipeInst->writeBuffer.buffer, 0, sizeof pipeInst->writeBuffer.buffer);
	pipeInst->writeBuffer.bufferSize = 0;

	pipes.insert(pipeInst);
	return pipeInst;
}

void IPCServer::ClosePipeInstance(PipeInstance* pipeInst)
{
	if (!DisconnectNamedPipe(pipeInst->pipe)) {
		LOG("DisconnectNamedPipe failed with %d.", GetLastError());
	}
	CloseHandle(pipeInst->pipe);
	pipes.erase(pipeInst);
	delete pipeInst;
}
*/

void IPCServer::RunThread(IPCServer* _this)
{
	_this->running = true;
	LPTSTR pipeName = TEXT(FREESCUBA_PIPE_NAME);

	HANDLE connectEvent = _this->connectEvent = CreateEvent(
		NULL,    // default security attribute
		TRUE,    // manual reset event 
		TRUE,    // initial state = signaled 
		NULL);   // unnamed event object 

	if (!connectEvent)
	{
		LOG("CreateEvent failed in RunThread. Error: %d", GetLastError());
		return;
	}

	OVERLAPPED connectOverlap = {};
	connectOverlap.hEvent = connectEvent;

	HANDLE nextPipe;
	BOOL connectPending = CreateAndConnectInstance(&connectOverlap, nextPipe);

	while (!_this->stop)
	{
		DWORD wait = WaitForSingleObjectEx(connectEvent, INFINITE, TRUE);

		if (_this->stop)
		{
			break;
		}
		else {
			switch (wait) {
			case 0:
			{
				// If an operation is pending, get the result of the 
				// connect operation. 
				if (connectPending) {
					DWORD bytesConnect;
					BOOL success;

					success = GetOverlappedResult(
						nextPipe,			// pipe handle 
						&connectOverlap,	// OVERLAPPED structure 
						&bytesConnect,		// bytes transferred 
						FALSE);				// does not wait

					if (!success)
					{
						LOG("ConnectNamedPipe failed in RunThread. Error: (%d)", GetLastError());
						return;
					}
				}

				LOG("IPC client connected");

				auto pipeInst = _this->CreatePipeInstance(nextPipe);
				pipeInst->writeBuffer.bufferSize = 0;
				CompletedWriteCallback(0, 0, (LPOVERLAPPED)pipeInst);

				connectPending = CreateAndConnectInstance(&connectOverlap, nextPipe);

				break;
			}
			case WAIT_IO_COMPLETION:
				break;
			default: {
				LOG("WaitForSingleObjectEx failed in RunThread. Error: (%d)", GetLastError());
				return;
			}
			}
		}

		/*
		else if (wait == 0)
		{
			// When connectPending is false, the last call to CreateAndConnectInstance
			// picked up a connected client and triggered this event, so we can simply
			// create a new pipe instance for it. If true, the client was still pending
			// connection when CreateAndConnectInstance returned, so this event was triggered
			// internally and we need to flush out the result, or something like that.
			if (connectPending)
			{
				DWORD bytesConnect;
				BOOL success = GetOverlappedResult(nextPipe, &connectOverlap, &bytesConnect, FALSE);
				if (!success)
				{
					LOG("GetOverlappedResult failed in RunThread. Error: %d", GetLastError());
					return;
				}
			}

			LOG("IPC client connected");

			auto pipeInst = _this->CreatePipeInstance(nextPipe);
			CompletedWriteCallback(0, sizeof protocol::Response, (LPOVERLAPPED)pipeInst);

			connectPending = CreateAndConnectInstance(&connectOverlap, nextPipe);
		}
		else if (wait != WAIT_IO_COMPLETION)
		{
			LOG("WaitForSingleObjectEx failed in RunThread. Error %d", GetLastError());
			return;
		}
		*/
	}

	for (auto& pipeInst : _this->pipes)
	{
		_this->ClosePipeInstance(pipeInst);
	}
	_this->pipes.clear();
}

/*
BOOL IPCServer::CreateAndConnectInstance(LPOVERLAPPED overlap, HANDLE& pipe)
{
	pipe = CreateNamedPipe(
		TEXT(FREESCUBA_PIPE_NAME),
		PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
		PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
		PIPE_UNLIMITED_INSTANCES,
		sizeof protocol::Request,
		sizeof protocol::Response,
		1000,
		0
	);

	if (pipe == INVALID_HANDLE_VALUE)
	{
		LOG("CreateNamedPipe failed. Error: %d", GetLastError());
		return FALSE;
	}

	ConnectNamedPipe(pipe, overlap);

	switch (GetLastError())
	{
	case ERROR_IO_PENDING:
		// Mark a pending connection by returning true, and when the connection
		// completes an event will trigger automatically.
		return TRUE;

	case ERROR_PIPE_CONNECTED:
		// Signal the event loop that a client is connected.
		if (SetEvent(overlap->hEvent))
			return FALSE;
	}

	LOG("ConnectNamedPipe failed. Error: %d", GetLastError());
	return FALSE;
}
*/

BOOL IPCServer::CreateAndConnectInstance(LPOVERLAPPED overlap, HANDLE& pipe)
{
	pipe = CreateNamedPipe(
		TEXT(FREESCUBA_PIPE_NAME),	// pipe name 
		PIPE_ACCESS_DUPLEX |      	// read/write access 
		FILE_FLAG_OVERLAPPED,     	// overlapped mode 
		PIPE_TYPE_MESSAGE |       	// message-type pipe 
		PIPE_READMODE_MESSAGE |   	// message read mode 
		PIPE_WAIT,                	// blocking mode 
		PIPE_UNLIMITED_INSTANCES, 	// unlimited instances 
		sizeof protocol::Request, 	// output buffer size 
		sizeof protocol::Response,	// input buffer size 
		1000,             			// client time-out 
		NULL);                    	// default security attributes

	if (pipe == INVALID_HANDLE_VALUE)
	{
		LOG("CreateNamedPipe failed with %d.", GetLastError());
		return 0;
	}

	// Call a subroutine to connect to the new client. 

	return ConnectToNewClient(pipe, overlap);
}

BOOL IPCServer::ConnectToNewClient(HANDLE hPipe, LPOVERLAPPED overlap)
{
	BOOL fConnected, fPendingIO = FALSE;

	// Start an overlapped connection for this pipe instance. 
	fConnected = ConnectNamedPipe(hPipe, overlap);

	// Overlapped ConnectNamedPipe should return zero. 
	if (fConnected) {
		LOG("ConnectNamedPipe failed with %d.", GetLastError());
		return 0;
	}

	switch (GetLastError()) {
		// The overlapped connection in progress. 
	case ERROR_IO_PENDING:
	{
		fPendingIO = TRUE;
		break;
	}
	// Client is already connected, so signal an event. 
	case ERROR_PIPE_CONNECTED:
	{
		if (SetEvent(overlap->hEvent))
			break;
	}
	// If an error occurs during the connect operation... 
	default:
	{
		LOG("ConnectNamedPipe failed with %d.", GetLastError());
		return 0;
	}
	}
	return fPendingIO;
}

void IPCServer::CompletedReadCallback(DWORD err, DWORD bytesRead, LPOVERLAPPED overlap)
{
	// lpOverlap points to storage for this instance.
	PipeInstance* pipeInst = (PipeInstance*)overlap;
	BOOL fWrite = FALSE;

	// The read operation has finished, so write a response (if no 
	// error occurred). 

	if ((err == 0) && (bytesRead != 0))
	{
		protocol::Request bufRequest = {};
		memcpy(&bufRequest, pipeInst->readBuffer.buffer, pipeInst->readBuffer.bufferSize);
		protocol::Response bufResponse = {};
		pipeInst->server->HandleRequest(bufRequest, bufResponse);
		memcpy(&bufResponse, pipeInst->writeBuffer.buffer, sizeof protocol::Response);

		fWrite = WriteFileEx(
			pipeInst->pipe,
			pipeInst->writeBuffer.buffer,
			pipeInst->writeBuffer.bufferSize,
			overlap,
			(LPOVERLAPPED_COMPLETION_ROUTINE)CompletedWriteCallback);
	}

	// Disconnect if an error occurred. 
	if (!fWrite) {
		LOG("IPC client disconnecting due to error (via CompletedReadCallback), error: %d, bytesRead: %d, lastError: %d", err, bytesRead, GetLastError());
		pipeInst->server->ClosePipeInstance(pipeInst);
	}
}

void IPCServer::CompletedWriteCallback(DWORD err, DWORD bytesWritten, LPOVERLAPPED overlap)
{
	// lpOverlap points to storage for this instance. 
	PipeInstance* pipeInst = (PipeInstance*)overlap;
	BOOL fRead = FALSE;

	// The write operation has finished, so read the next request (if 
	// there is no error). 
	LOG("I HATE NAMED PIPES");
	if ((err == 0) && (bytesWritten == pipeInst->readBuffer.bufferSize)) {
		LOG("Reading file...");
		fRead = ReadFileEx(
			pipeInst->pipe,
			pipeInst->readBuffer.buffer,
			pipeInst->readBuffer.bufferSize,
			overlap,
			(LPOVERLAPPED_COMPLETION_ROUTINE)CompletedReadCallback);
		LOG("Got return code %d", fRead);
	}

	// Disconnect if an error occurred. 
	if (!fRead) {
		LOG("IPC client disconnecting due to error (via CompletedWriteCallback), error: %d, bytesWritten: %d, lastErr: %d", err, bytesWritten, GetLastError());
		pipeInst->server->ClosePipeInstance(pipeInst);
	}
}
#endif