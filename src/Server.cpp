#include "Server.hpp"

#include "tools.hpp"

#include <Network/URL.hpp>
#include <Network/HTTP.hpp>
#include <Network/TCPServer.hpp>

#include <fstream>
#include <sstream>
#include <string>
#include <memory>
#include <vector>

#include <wolfssl/options.h>
#include <wolfssl/ssl.h>


#ifdef _WIN32
	#define WIN(exp) exp
	#define NIX(exp)
#else
	#define WIN(exp)
	#define NIX(exp) exp
#endif


namespace app
{

Server::Server(const std::string& certs_dir, const std::string& res_dir):
	m_certificates_directory(certs_dir),
	m_resources_directory(res_dir),
	net::TCPServer()
{
	wolfSSL_Init();

	m_ctx = wolfSSL_CTX_new(wolfTLSv1_2_server_method());
	if (!m_ctx)
	{
		std::cerr << "Failed to create SSL_CTX" << std::endl;
		return;
	}

	if (wolfSSL_CTX_use_certificate_chain_file(m_ctx, std::string(m_certificates_directory + "full_certificate_chain.pem").c_str()) != SSL_SUCCESS)
		std::cerr << "Failed to load certificate PEM" << std::endl;
	if (wolfSSL_CTX_use_PrivateKey_file(m_ctx, std::string(m_certificates_directory + "private_key.pem").c_str(), SSL_FILETYPE_PEM) != SSL_SUCCESS)
		std::cerr << "Failed to load private key PEM" << std::endl;
}

Server::~Server()
{
	if (m_ctx)
	{
		wolfSSL_CTX_free(m_ctx);
		m_ctx = nullptr;
	}

	wolfSSL_Cleanup();
}



void app::Server::request_handler(int client_socket)
{
	WOLFSSL* ssl = wolfSSL_new(m_ctx);
	if (!ssl)
	{
		std::cerr << "wolfSSL_new failed on socket " << client_socket << std::endl;
		return;
	}
	m_ssl_map[client_socket] = ssl;
	wolfSSL_set_fd(ssl, client_socket);
	WIN(
		u_long mode = 0;
		ioctlsocket(client_socket, FIONBIO, &mode);
	)
	NIX(
		int flags = fcntl(client_socket, F_GETFL, 0);
		fcntl(client_socket, F_SETFL, flags & ~O_NONBLOCK);
	)

	int accept_res = wolfSSL_accept(ssl);
	if (accept_res != SSL_SUCCESS)
	{
		std::cerr << "TLS handshake failed on socket " << client_socket << std::endl;
		wolfSSL_free(ssl);
		return;
	}

	session_handler(client_socket);

	wolfSSL_free(ssl);
	m_ssl_map.erase(client_socket);
}

void app::Server::session_handler(int client_socket)
{
	net::HTTPResponse response;
	net::HTTPRequest req(this->recv(client_socket));
	net::URI uri(req.start_line[1]);
	std::string path = uri.toString(false);

	if (path.find(".") == std::string::npos)
	{
		char last_char = path[(path.size()) ? (path.size() - 1) : 0];
		path += (last_char == '/') ? "index.html" : "/index.html";
	}

	if (req.headers.find("Range") == req.headers.end())
		if (tools::get_file_size(m_resources_directory + path) < 1024*1024*100) //100MB
			tools::full_file_load_handler(response, m_resources_directory, path);
		else
		{
			req.headers["Range"] = "bytes=0-";
			tools::range_file_load_handler(req, response, m_resources_directory, path);
		}
	else
		tools::range_file_load_handler(req, response, m_resources_directory, path);

	
	response.start_line[0] = "HTTP/1.1";
	
	response.headers["Version"] = "HTTP/1.1";
	response.headers["Content-Type"] = tools::get_content_type(path);
	response.headers["Content-Length"] = std::to_string(response.body.length());

	this->send(client_socket, response.toString());
}


std::string app::Server::recv_handler(int socket)
{
    auto it = m_ssl_map.find(socket);
    if (it == m_ssl_map.end())
    {
        std::cerr << "No SSL object for socket " << socket << std::endl;
        return "";
    }
    WOLFSSL* ssl = it->second;

    static constexpr int max_buffer = 1024;
    char buf[max_buffer];
    std::string result;
    int content_length = -1;

    while (true)
    {
        int n = wolfSSL_read(ssl, buf, max_buffer);
        if (n > 0)
        {
            result.append(buf, n);
            if (tools::is_request_complete(result, content_length))
                break;
        }
        else if (n == 0)
            break;
        else
        {
            int err = wolfSSL_get_error(ssl, n);
            std::cerr << "wolfSSL_read error: " << err << std::endl;
            return "";
        }
    }
    return result;
}

void app::Server::send_handler(int socket, const std::string& message)
{
	auto it = m_ssl_map.find(socket);
	if (it == m_ssl_map.end())
	{
		std::cerr << "No SSL object for socket " << socket << std::endl;
		return;
	}
	WOLFSSL* ssl = it->second;

	int res = wolfSSL_write(ssl, message.c_str(), int(message.size()));
	if (res != (int)message.size())
		std::cerr << "wolfSSL_write failed on socket " << socket << std::endl;
}

} //namespace app