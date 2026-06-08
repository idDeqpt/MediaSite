#include <iostream>
#include <string>

#include <Network/ServerSessionData.hpp>
#include <Network/Timer.hpp>
#include "Server.hpp"


#ifdef _WIN32
	#define WIN(exp) exp
	#define NIX(exp)
#else
	#define WIN(exp)
	#define NIX(exp) exp
#endif


int main(int argc, char* argv[])
{
	std::string path = argv[0];
	int last_slash = path.rfind(
		WIN("\\")
		NIX("/")
	);

	int port = -1;
	if (argc > 1)
		try
		{
			port = std::stoi(std::string(argv[1]));
			std::cout << "Port: " << port << std::endl;
		} catch (...) {}

	while (port < 0)
	{
		std::cout << "Enter the port: ";
		std::string port_s;
		std::cin >> port_s;
		try
		{
			port = std::stoi(port_s);
		}
		catch (...)
		{
			std::cout << "Unexpected character!" << std::endl;
		}
	}

	app::Server server(path.substr(0, last_slash + 1));
	int init_status = server.init(port);

	if (!server.start())
	{
		std::cout << "Server start incompleted: " << init_status << std::endl;
		return 0;
	}
	std::cout << "Server start completed\n------------------------------------------------------------------------------------------\n";

	Timer timer;
	while(true)
	{
		if (server.hasNewSessionData())
		{
			static constexpr char* session_data_types[] = {"Open", "Close", "RECV", "Send"};
			net::ServerSessionData session_data = server.getNextSessionData();
			std::string data_str = session_data.getText();
			if (data_str.length() > 1024) data_str.resize(1024);
			std::cout << session_data_types[session_data.getType()] << " " << session_data.getId() << " " << session_data.getTime() << "s:" << std::endl
			<< data_str << std::endl
			<< "==========================================================================================\n";
		}
		timer.sleep(50);
	}

	system("pause");
	return 0;
}