#ifndef APP_HTTPS_SERVER_CLASS_HEADER
#define APP_HTTPS_SERVER_CLASS_HEADER

#include "HTTPServer.hpp"

#include <unordered_map>
#include <string>

struct WOLFSSL_CTX;
struct WOLFSSL;

namespace app
{
	class HTTPSServer : public HTTPServer
	{
	public:
		HTTPSServer(const std::string& certs_dir, const std::string& res_dir);
		~HTTPSServer();

	protected:
		std::string  m_certificates_directory;
		WOLFSSL_CTX* m_ctx;
		std::unordered_map<int, WOLFSSL*> m_ssl_map;

		void request_handler(int client_socket) override;

		std::string recv_handler(int socket) override;
		void send_handler(int socket, const std::string& message) override;
	};
}

#endif //APP_HTTPS_SERVER_CLASS_HEADER