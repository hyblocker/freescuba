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
			pipe->writeBuffer.dataSize = sizeof(protocol::Response_t);
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
					static_cast<DWORD>(lpPipeInst->writeBuffer.dataSize),
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