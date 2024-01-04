#include "../includes/Client.hpp"
#include "../includes/Server.hpp"
#include "../includes/Request.hpp"

Client::Client() {}

Client::Client(int client_socket)
	: socket_fd(client_socket), status(RECV_REQUEST) {}

Client::~Client() {}

int Client::getSocketFd() { return socket_fd; }

int Client::getStatus() { return status; }

void	Client::setStatus(Status status) { this->status = status; }

void	Client::setServer(Server server) { this->server = server; }

void    Client::findLocation()
{
	m_location = server.getLoc()[request.getUri()];

	// std::cout << "---------- " << std::endl;
	// for (std::map<std::string, std::string>::iterator it = m_location.begin(); it != m_location.end(); ++it)
	// 	std::cout << it->first << " : " << it->second << std::endl;
	// std::cout << "---------- " << std::endl;
}

void Client::HandleSocketRead()
{
	Request Req;
	char buf[1024];
	int msg_length = read(this->socket_fd, buf, 1024);
	(void)msg_length; // make를 위한 void입니당 지워도됩니당
	Req.ReqParsing(buf);
}

void Client::HandleSocketWrite()
{
	//response = Response();
	written = 0;
	const std::vector<char>& send_buffer = response.getSendBuffer();
	ssize_t write_size = send_buffer.size() - written > 1024 ? 1024 : send_buffer.size() - written;
	write_size = write(socket_fd, &send_buffer[written], write_size);
	if (write_size == -1) { //write 오류
		status = DISCONNECT;
		return;
	}
	written += write_size;
	if (written == static_cast<ssize_t>(send_buffer.size())) //다쓰면 연결해제
		status = DISCONNECT;	
}
