#include "ipc_client.hpp"
#include <stdexcept>
#include <string>

std::string WStringToString(const std::wstring& wstr)
{
	int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
	std::string str_to(size_needed, 0);
	WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &str_to[0], size_needed, NULL, NULL);
	return str_to;
}

static std::string LastErrorString(DWORD lastError)
{
	LPWSTR buffer = nullptr;
	size_t size = FormatMessageW(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, lastError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&buffer, 0, NULL
	);
	std::wstring message(buffer, size);
	LocalFree(buffer);
	return WStringToString(message);
}

IPCClient::~IPCClient()
{
	if (pipe && pipe != INVALID_HANDLE_VALUE)
		CloseHandle(pipe);
}

void IPCClient::Connect()
{
	// Connect to pipe job
	while (1)
	{
		LPTSTR pipeName = (LPTSTR)TEXT(FREESCUBA_PIPE_NAME);
		pipe = CreateFile(
			pipeName,   // pipe name 
			GENERIC_READ |  // read and write access 
			GENERIC_WRITE,
			0,              // no sharing 
			NULL,           // default security attributes
			OPEN_EXISTING,  // opens existing pipe 
			0,              // default attributes 
			NULL);          // no template file 

		// Break if the pipe handle is valid. 
		if (pipe != INVALID_HANDLE_VALUE)
			break;

		// Exit if an error other than ERROR_PIPE_BUSY occurs. 
		if (GetLastError() != ERROR_PIPE_BUSY) {
			throw std::runtime_error(std::string("Could not open pipe. Got error ") + std::to_string(GetLastError()));
		}

		// All pipe instances are busy, so wait for 20 seconds. 
		if (!WaitNamedPipe(pipeName, 20000)) {
			throw std::runtime_error("Could not open pipe: 20 second wait timed out.");
		}
	}


	DWORD mode = PIPE_READMODE_MESSAGE;
	if (!SetNamedPipeHandleState(pipe, &mode, 0, 0))
	{
		DWORD lastError = GetLastError();
		throw std::runtime_error("Couldn't set pipe mode. Error " + std::to_string(lastError) + ": " + LastErrorString(lastError));
	}

	auto response = SendBlocking(protocol::Request(protocol::RequestHandshake));
	if (response.type != protocol::ResponseHandshake || response.protocol.version != protocol::Version)
	{
		throw std::runtime_error(
			"Incorrect driver version installed, try reinstalling FreeScuba. (Client: " +
			std::to_string(protocol::Version) +
			", Driver: " +
			std::to_string(response.protocol.version) +
			")"
		);
	}
}

protocol::Response IPCClient::SendBlocking(const protocol::Request& request)
{
	Send(request);
	return Receive();
}

void IPCClient::Send(const protocol::Request& request)
{
	DWORD bytesWritten;
	BOOL success = WriteFile(pipe, &request, sizeof request, &bytesWritten, 0);
	if (!success)
	{
		DWORD lastError = GetLastError();
		throw std::runtime_error("Error writing IPC request. Error " + std::to_string(lastError) + ": " + LastErrorString(lastError));
	}
}

protocol::Response IPCClient::Receive()
{
	protocol::Response response(protocol::ResponseInvalid);
	DWORD bytesRead;

	BOOL success = ReadFile(pipe, &response, sizeof response, &bytesRead, 0);
	if (!success)
	{
		DWORD lastError = GetLastError();
		if (lastError != ERROR_MORE_DATA)
		{
			throw std::runtime_error("Error reading IPC response. Error " + std::to_string(lastError) + ": " + LastErrorString(lastError));
		}
	}

	if (bytesRead != sizeof response)
	{
		throw std::runtime_error("Invalid IPC response with size " + std::to_string(bytesRead));
	}

	return response;
}