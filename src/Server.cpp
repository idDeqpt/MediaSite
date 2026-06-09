#include "Server.hpp"

#include <Network/URL.hpp>
#include <Network/HTTP.hpp>
#include <Network/TCPServer.hpp>
#include <Network/ServerSessionData.hpp>
#include <Network/Timer.hpp>

#include <fstream>
#include <sstream>
#include <string>
#include <memory>
#include <vector>

#include <wolfssl/options.h>
#include <wolfssl/ssl.h>
#include <wolfssl/wolfcrypt/pkcs12.h>


#ifdef _WIN32
	#define WIN(exp) exp
	#define NIX(exp)
#else
	#define WIN(exp)
	#define NIX(exp) exp
#endif


namespace app
{

struct ByteRange
{
	unsigned int start;
	unsigned int end;
	unsigned int total;
};


bool is_request_complete(const std::string& data, int& content_length);

std::string get_content_type(const std::string& path);
unsigned int get_file_size(const std::string& path);
ByteRange get_byte_range(const std::string& range_string, unsigned int file_size);

std::unique_ptr<std::string> load_file_data_ptr(const std::string& path);
std::unique_ptr<std::string> partially_load_file_data_ptr(const std::string& path, ByteRange byte_range);



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
	{
		std::unique_ptr<std::string> data_ptr = load_file_data_ptr(m_resources_directory + path);
		if (data_ptr == nullptr)
		{
			response.start_line[1] = "404";
			response.start_line[2] = "NOT FOUND";
			data_ptr = load_file_data_ptr(m_resources_directory + "404/index.html");
		}
		else
		{
			response.start_line[1] = "200";
			response.start_line[2] = "OK";
		}
		response.body = *data_ptr;
	}
	else
	{
		std::string file_path = m_resources_directory + path;
		ByteRange range = get_byte_range(req.headers["Range"], get_file_size(file_path));
		std::unique_ptr<std::string> data_ptr = partially_load_file_data_ptr(file_path, range);
		if (data_ptr == nullptr)
		{
			response.start_line[1] = "404";
			response.start_line[2] = "NOT FOUND";
			data_ptr = load_file_data_ptr(m_resources_directory + "404/index.html");
		}

		response.start_line[1] = "206";
		response.start_line[2] = "PARTIAL CONTENT";
		response.headers["Connection"] = "keep-alive";
		response.headers["Cache-Control"] = "no-cache";
		response.headers["Accept-Ranges"] = "bytes";
		response.headers["Content-Range"] = "bytes " + std::to_string(range.start) + "-" + std::to_string(range.end) + "/" + std::to_string(range.total);
		response.body = *data_ptr;
	}

	
	response.start_line[0] = "HTTP/1.1";
	
	response.headers["Version"] = "HTTP/1.1";
	response.headers["Content-Type"] = get_content_type(path);
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
            if (is_request_complete(result, content_length))
                break;
        }
        else if (n == 0)
            break; // клиент закрыл соединение
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



bool is_request_complete(const std::string& data, int& content_length)
{
	if (content_length == -1) {
		unsigned int pos = data.find("Content-Length:");
		if (pos == std::string::npos)
			content_length = 0;
		else {
			pos += 15;
			unsigned int end = data.find("\r\n", pos);
			if (end != std::string::npos)
				content_length = std::stoi(data.substr(pos, end - pos));
		}
	}
	if (content_length == 0) {
		return data.find("\r\n\r\n") != std::string::npos;
	} else {
		unsigned int headers_end = data.find("\r\n\r\n");
		if (headers_end == std::string::npos)
			return false;
		unsigned int body_received = data.size() - headers_end - 4;
		return body_received >= (unsigned int)content_length;
	}
}


std::string get_content_type(const std::string& path)
{
	if (path.find(".html") != std::string::npos)
		return "text/html; charset=utf-8";
	if (path.find(".css") != std::string::npos)
		return "text/css; charset=utf-8";
	if (path.find(".js") != std::string::npos)
		return "application/javascript; charset=utf-8";
	if (path.find(".png") != std::string::npos)
		return "image/png";
	if (path.find(".mp4") != std::string::npos)
		return "video/mp4";
	if (path.find(".mkv") != std::string::npos)
		return "video/x-motroska";
	return "text/plane; charset=utf-8";
}

unsigned int get_file_size(const std::string& path)
{
	std::ifstream file(path, std::ios::binary | std::ios::ate);

	return (!file) ? 0 : file.tellg();
}

ByteRange get_byte_range(const std::string& range_string, unsigned int file_size)
{
	static const unsigned int max_buffer = 1024*512;
	std::string data = range_string.substr(6); //skip "bytes="

	ByteRange br = {0, 0, file_size};
	if (data.front() == '-') //last N bytes
	{
		int count = stoi(data.substr(1));
		br.start = (count < file_size) ? (file_size - count - 1) : 0;
		br.end = file_size - 1;
	}
	else if (data.back() == '-') //all bytes starting from N
	{
		int first = stoi(data.substr(0, data.length() - 1));
		br.start = (first < file_size) ? first : (file_size - 1);
		br.end = file_size - 1;
	}
	else //bytes from Left to Right values
	{
		unsigned int mark = data.find('-');
		unsigned int start = stoi(data.substr(0, mark));
		unsigned int end = stoi(data.substr(mark + 1));
		br.start = (start < file_size) ? start : (file_size - 1);
		br.end = (end < br.start) ? (br.start + 1) : ((end < file_size) ? end : (file_size - 1));
	}
	br.end = ((br.end - br.start) < max_buffer) ? br.end : (br.start + max_buffer);

	return br;
}


std::unique_ptr<std::string> load_file_data_ptr(const std::string& path)
{
	std::ifstream file(path, std::ios::binary);

	if (!file)
		return std::unique_ptr<std::string>(nullptr);

	std::ostringstream oss;
	oss << file.rdbuf();
	file.close();
	return std::make_unique<std::string>(oss.str());
}

std::unique_ptr<std::string> partially_load_file_data_ptr(const std::string& path, ByteRange byte_range)
{
	std::ifstream file(path, std::ios::binary);

	if (!file)
		return std::unique_ptr<std::string>(nullptr);

	file.seekg(byte_range.start, std::ios::beg);
	
	std::string result(byte_range.end - byte_range.start + 1, '\0');
	file.read(&(result)[0], byte_range.end - byte_range.start + 1);

	result.resize(file.gcount());

	file.close();
	return std::make_unique<std::string>(result);
}

} //namespace app