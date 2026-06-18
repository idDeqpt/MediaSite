#include "HTTPServer.hpp"

#include "tools.hpp"

#include <Network/URL.hpp>
#include <Network/HTTP.hpp>
#include <Network/TCPServer.hpp>

#include <string>


namespace app
{

HTTPServer::HTTPServer(const std::string& res_dir):
	m_resources_directory(res_dir),
	net::TCPServer() {}


void app::HTTPServer::request_handler(int client_socket)
{
	session_handler(client_socket);
}

void app::HTTPServer::session_handler(int client_socket)
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

} //namespace app