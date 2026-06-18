#include <iostream>
#include <string>

#include <Network/ServerSessionData.hpp>
#include <Network/Timer.hpp>
#include "Server.hpp"
#include "CommandLineOptions.hpp"


#ifdef _WIN32
	#define WIN(exp) exp
	#define NIX(exp)
#else
	#define WIN(exp)
	#define NIX(exp) exp
#endif


int main(int argc, char* argv[])
{
	app::cli::OptionName certificates_option('c', "certificates");
	app::cli::OptionName port_option('p', "port");
	app::cli::OptionName resources_option('r', "resources");

	app::cli::Options cli_options;
	bool res = cli_options.parse(argc, argv, {
		{certificates_option, app::cli::OptionType::ONE_ARGUMENT},
		{port_option,         app::cli::OptionType::ONE_ARGUMENT},
		{resources_option,    app::cli::OptionType::ONE_ARGUMENT}
	});
	if (res == false)
	{
		std::cout << "Command line reading error!" << std::endl;
		system("pause");
		return -1;
	}

	unsigned int argn = 1;

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

	if (!use_tls)
	{
		std::cout << "Server does not support running without TLS certificates!" << std::endl;
		system("pause");
		return -2;
	}

	app::Server server(certs_dir, res_dir);
	int init_status = server.init(port);

	if (!server.start())
	{
		std::cout << "Server start incompleted: " << init_status << std::endl;
		return 0;
	}
	std::cout << "Server start completed on port " << port << " " << (use_tls ? "with" : "without") << " TLS" << "\n------------------------------------------------\n";

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