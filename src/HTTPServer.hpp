#ifndef APP_HTTP_SERVER_CLASS_HEADER
#define APP_HTTP_SERVER_CLASS_HEADER

#include <Network/TCPServer.hpp>
#include <string>

namespace app
{
	class HTTPServer : public net::TCPServer
	{
	public:
		HTTPServer(const std::string& res_dir);
		virtual ~HTTPServer() = default;

	protected:
		std::string  m_resources_directory;

		virtual void request_handler(int client_socket) override;
		void session_handler(int client_socket);
	};
}

#endif //APP_HTTP_SERVER_CLASS_HEADER