#include <iostream>
#include <string>

#include <Network/ServerSessionData.hpp>
#include <Network/Timer.hpp>
#include <Network/WebServer.hpp>
#include <Network/WebServerSecure.hpp>

#include <CLI/CommandLineOptions.hpp>


int main(int argc, char* argv[])
{
	cli::OptionName certificates_option('c', "certificates");
	cli::OptionName port_option('p', "port");
	cli::OptionName resources_option('r', "resources");

	cli::Options cli_options;
	bool res = cli_options.parse(argc, argv, {
		{certificates_option, cli::OptionType::ONE_ARGUMENT},
		{port_option,         cli::OptionType::ONE_ARGUMENT},
		{resources_option,    cli::OptionType::ONE_ARGUMENT}
	});
	if (res == false)
	{
		std::cout << "Command line reading error!" << std::endl;
		system("pause");
		return -1;
	}

	const std::string* argument = cli_options.getArgument(certificates_option);
	std::string certs_dir = "";
	bool use_tls = false;
	if (argument != nullptr)
	{
		use_tls = true;
		certs_dir = *argument;
		if (certs_dir.size() && (certs_dir[certs_dir.size() - 1] != '/')) certs_dir += '/';
	}

	argument = cli_options.getArgument(port_option);
	int port = -1;
	if (argument != nullptr)
		try
		{
			port = std::stoi(*argument);
		} catch (...) {}
	if (port == -1)
		port = (use_tls) ? 443 : 80;

	argument = cli_options.getArgument(resources_option);
	std::string res_dir = "";
	if (argument != nullptr)
	{
		res_dir = *argument;
		if (res_dir.size() && (res_dir[res_dir.size() - 1] != '/')) res_dir += '/';
	}

	net::TCPServer* server = (use_tls) ?
		new net::WebServerSecure(certs_dir, res_dir) :
		new net::WebServer(res_dir);

	int init_status = server->init(port);

	if (!server->start())
	{
		std::cout << "Server start incompleted! Status: " << init_status << std::endl;
		return -2;
	}
	std::cout << "Server start completed on port " << port << " " << (use_tls ? "with" : "without") << " TLS" << "\n------------------------------------------------\n";

	Timer timer;
	while(true)
	{
		if (server->hasNewSessionData())
		{
			static constexpr char* session_data_types[] = {"Open", "Close", "RECV", "Send"};
			net::ServerSessionData session_data = server->getNextSessionData();
			std::string data_str = session_data.getText();
			if (data_str.length() > 1024) data_str.resize(1024);
			std::cout << session_data_types[session_data.getType()] << " " << session_data.getId() << " " << session_data.getTime() << "s:" << std::endl
			<< data_str << std::endl
			<< "==========================================================================================\n";
		}
		timer.sleep(50);
	}

	delete server;

	system("pause");
	return 0;
}