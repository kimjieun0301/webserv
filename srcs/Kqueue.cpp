#include "../includes/Kqueue.hpp"
#include "../includes/Config.hpp"
#include "../includes/Client.hpp"

Kqueue::Kqueue() {}

Kqueue::~Kqueue() {}

void	Kqueue::initServer(Config conf)
{
	if ((kq = kqueue()) == -1)
		throw "kqueue() error";

	std::vector<Server> servs = conf.getConfig();
	for (std::vector<Server>::iterator it = servs.begin(); it != servs.end(); ++it)
	{
		int server_socket;
		struct sockaddr_in server_addr;

		if ((server_socket = socket(PF_INET, SOCK_STREAM, 0)) == -1)
			throw "socket() error";

		memset(&server_addr, 0, sizeof(server_addr));
		server_addr.sin_family = AF_INET;
		server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		server_addr.sin_port = htons(it->getPort());

		if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1)
			throw "bind() error";
		if (listen(server_socket, 3) == -1)
			throw "listen() error";
		fcntl(server_socket, F_SETFL, O_NONBLOCK);

		change_events(change_list, server_socket, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
		v_server.push_back(server_socket);

		std::cout << "[server start] " << it->getName() << ":" << it->getPort() << std::endl;
	}
}

void Kqueue::change_events(std::vector<struct kevent> &change_list, uintptr_t ident, int16_t filter,
							uint16_t flags, uint32_t fflags, intptr_t data, void *udata)
{
	struct kevent temp_event;

	EV_SET(&temp_event, ident, filter, flags, fflags, data, udata);
	change_list.push_back(temp_event);
}

void Kqueue::disconnect_client(int client_fd)
{
	close(client_fd);
	for (std::vector<Client *>::iterator it; it != v_client.end(); ++it)
	{
		if ((*it)->getFd() == client_fd)
		{
			v_client.erase(it);
			delete *it;
			break;
		}
	}
	std::cout << "[disconnect client]" << std::endl;
}

void Kqueue::connect_client(int client_fd)
{
	int client_socket;
	if ((client_socket = accept(client_fd, NULL, NULL)) == -1)
		throw "accept() error";
	fcntl(client_socket, F_SETFL, O_NONBLOCK);

	change_events(change_list, client_socket, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
	change_events(change_list, client_socket, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, NULL);
	
	Client *client = new Client(client_socket);
	v_client.push_back(client);
	std::cout << "[connect new client]" << std::endl;
}

bool Kqueue::isServer(int fd)
{
	if (std::find(v_server.begin(), v_server.end(), fd) != v_server.end())
		return true;
	else
		return false;
}

bool Kqueue::isClient(int fd)
{
	for (std::vector<Client *>::iterator it = v_client.begin(); it != v_client.end(); ++it)
	{
		if ((*it)->getFd() == fd)
			return true;
	}
	return false;
}

void	Kqueue::startServer()
{
	int new_events;
	struct kevent* curr_event;

	while (1)
	{
		new_events = kevent(kq, &change_list[0], change_list.size(), event_list, 8, NULL);
		if (new_events == -1)
			throw "kevent error";
		change_list.clear();

		for (int i = 0; i < new_events; ++i)
		{
			curr_event = &event_list[i];
			if (curr_event->flags & EV_ERROR)
			{
				if (isServer(curr_event->ident))
					throw "server socket error";
				else
					disconnect_client(curr_event->ident);
			}
			else if (curr_event->filter == EVFILT_READ)
			{
				if (isServer(curr_event->ident))
					connect_client(curr_event->ident);
				else if (isClient(curr_event->ident))
				{
					// client status 보고 read 작업. (HandleSocketRead 등)
				}
			}
			else if (curr_event->filter == EVFILT_WRITE)
			{
				if (isClient(curr_event->ident))
				{
					// HandleSocketWrite 등
				}
			}
		}
	}
}