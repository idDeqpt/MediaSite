#ifndef APP_SERVER_CLASS_HEADER
#define APP_SERVER_CLASS_HEADER

#include <Network/TCPServer.hpp>
#include <unordered_map>
#include <string>

struct WOLFSSL_CTX;
struct WOLFSSL;

namespace app
{
	class Server : public net::TCPServer
	{
	public:
		Server(const std::string& certs_dir, const std::string& res_dir);
		~Server();

	protected:
		std::string  m_resources_directory;
		std::string  m_certificates_directory;
		WOLFSSL_CTX* m_ctx;
		std::unordered_map<int, WOLFSSL*> m_ssl_map;

		void request_handler(int client_socket) override;
		void session_handler(int client_socket);

		std::string recv_handler(int socket) override;
		void send_handler(int socket, const std::string& message) override;
	};
}

#endif //APP_SERVER_CLASS_HEADER