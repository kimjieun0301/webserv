#include "../includes/Kqueue.hpp"
#include "../includes/Config.hpp"
#include "../includes/Client.hpp"
#include "../includes/Event.hpp"

Kqueue::Kqueue() {}

Kqueue::~Kqueue() {}

void	Kqueue::initServer(Config &config)
{
	if ((kq = kqueue()) == -1)
		throw "kqueue() error";

	v_config = config.getServer();
	for (std::vector<Server *>::iterator it = v_config.begin(); it != v_config.end(); ++it)
	{
		int server_socket;
		struct sockaddr_in server_addr;

		if ((server_socket = socket(PF_INET, SOCK_STREAM, 0)) == -1)
			throw "socket() error";

		memset(&server_addr, 0, sizeof(server_addr));
		server_addr.sin_family = AF_INET;
		server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		server_addr.sin_port = htons((*it)->getPort());

		if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1)
			throw "bind() error";
		if (listen(server_socket, 3) == -1)
			throw "listen() error";
		fcntl(server_socket, F_SETFL, O_NONBLOCK);

		Event::changeEvents(change_list, server_socket, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
		(*it)->setSocketFd(server_socket);
		v_server.push_back(server_socket);

		std::cout << GREEN << "[server start] " << (*it)->getName() << ":" << (*it)->getPort() << RESET << std::endl;
	}
}

void Kqueue::disconnectClient(int client_fd)
{
	Event::changeEvents(change_list, client_fd, EVFILT_WRITE, EV_DISABLE, 0, 0, NULL);
	for (std::vector<Client *>::iterator it = v_client.begin(); it != v_client.end(); ++it)
	{
		if ((*it)->socket_fd == client_fd)
		{
			v_client.erase(it);
			delete *it;
			break;
		}
	}
	close(client_fd);
	std::cout << "[disconnect client] " << client_fd << std::endl;
}

void Kqueue::connectClient(int server_fd)
{
	int client_socket;
	if ((client_socket = accept(server_fd, NULL, NULL)) == -1)
		throw "accept() error";
	fcntl(client_socket, F_SETFL, O_NONBLOCK);

	Event::changeEvents(change_list, client_socket, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
	
	Client *client = new Client(client_socket);
	for (std::vector<Server *>::iterator it = v_config.begin(); it != v_config.end(); ++it)
	{
		if ((*it)->getSocketFd() == server_fd)
		{
			client->server = *it;
			break;
		}
	}
	v_client.push_back(client);
	std::cout << "[connect new client] " << client_socket << std::endl;
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
		if ((*it)->socket_fd == fd)
			return true;
		else if ((*it)->file_fd == fd)
			return true;
	}
	return false;
}

Client* Kqueue::getClient(int fd)
{
	std::vector<Client *>::iterator it;
	for (it = v_client.begin(); it != v_client.end(); ++it)
	{
		if ((*it)->socket_fd == fd)
			break;
		else if ((*it)->file_fd == fd)
			break;
	}
	return *it;
}

void	Kqueue::startServer()
{
	int new_events;
	struct kevent event_list[8];
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
				// else if (isClient(curr_event->ident))
				// 	disconnectClient(curr_event->ident);
			}
			else if (curr_event->filter == EVFILT_READ)
			{
				if (isServer(curr_event->ident))
					connectClient(curr_event->ident);
				else if (isClient(curr_event->ident))
				{
					Client *client = getClient(curr_event->ident);
					switch ((int)client->status)
					{
					case READ_SOCKET:
						Event::readSocket(*client, change_list);
						Event::checkMethod(*client, change_list);
						break;
					case READ_FILE: 
						Event::readFile(*client, change_list);
						break;
					case DISCONNECT:
						disconnectClient(client->socket_fd);
						break;
					}
				}
			}
			else if (curr_event->filter == EVFILT_WRITE)
			{
				if (isClient(curr_event->ident))
				{
					Client *client = getClient(curr_event->ident);
					switch ((int)client->status)
					{
					case WRITE_SOCKET:
						Event::writeSocket(*client, change_list);
						break;
					case DISCONNECT:
						disconnectClient(client->socket_fd);
						break;
					}
				}
			}
		}
	}
}
