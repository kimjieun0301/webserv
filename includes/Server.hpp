#ifndef SERVER_HPP
#define SERVER_HPP

#include "FdSet.hpp"
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <map>
#include <vector>

class Location;

// : public FdSet
class Server
{
	private: 
		std::string name;
		std::string ip; // 아직
		std::string port;
		std::map<std::string, std::string> serv; // 아직
		std::map< std::string, std::map<std::string, std::string> > loc; // std::map<std::string, Location> ???

	public:
		Server();
		virtual ~Server();

		std::string	getName() const;
		std::string	getIP() const;
		std::string	getPort() const;
		std::map< std::string, std::map<std::string, std::string> > getLoc() const;

		void	parseDirective(const std::string& dir, const std::string& line);
		void	parseLocation(std::ifstream& file, std::string& line);
};

#endif
