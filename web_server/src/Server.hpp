#ifndef APP_SERVER_CLASS_HEADER
#define APP_SERVER_CLASS_HEADER

#include <Network/TCPServer.hpp>
#include <string>

namespace app
{
	class Server : public net::TCPServer
	{
	public:
		Server(const std::string& work_dir);

		const std::string& getWorkDirectory();

	protected:
		std::string m_work_directory;
	};
}

#endif //APP_SERVER_CLASS_HEADER