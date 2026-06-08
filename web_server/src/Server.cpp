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


namespace app
{

struct ByteRange
{
	unsigned int start;
	unsigned int end;
	unsigned int total;
};

std::string get_content_type(const std::string& path);
unsigned int get_file_size(const std::string& path);
ByteRange get_byte_range(const std::string& range_string, unsigned int file_size);

std::unique_ptr<std::string> load_file_data_ptr(const std::string& path);
std::unique_ptr<std::string> partially_load_file_data_ptr(const std::string& path, ByteRange byte_range);

void request_handler(net::TCPServer* server, int client_socket);



Server::Server(const std::string& work_dir):
	m_work_directory(work_dir),
	net::TCPServer()
{
	setRequestHandler(app::request_handler);
}

const std::string& Server::getWorkDirectory()
{
	return m_work_directory;
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
	static const unsigned int chunk_buffer = 1024*64;
	static const unsigned int max_buffer = 1024*1024;
	std::string data = range_string.substr(6);

	ByteRange br = {0, 0, file_size};
	if (data.front() == '-')
	{
		int num = stoi(data.substr(1));
		num = (num > chunk_buffer) ? chunk_buffer : num;
		br.start = (num < file_size) ? (file_size - num - 1) : 0;
		br.end = file_size - 1;
	}
	else if (data.back() == '-')
	{
		int num = stoi(data.substr(0, data.length() - 1));
		br.start = num;
		br.end = ((num + chunk_buffer - 1) < file_size) ? (num + chunk_buffer - 1) : (file_size - 1);
	}
	else
	{
		unsigned int mark = data.find('-');
		unsigned int start = stoi(data.substr(0, mark));
		unsigned int end = stoi(data.substr(mark + 1));
		br.start = (start < file_size) ? start : (file_size - 1);
		br.end = (end < file_size) ? end : (file_size - 1);
	}
	int total_bytes = br.end - br.start;
	br.end = (total_bytes < max_buffer) ? br.end : (br.start + max_buffer - 1);

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


void request_handler(net::TCPServer* server, int client_socket)
{
	Server* s = static_cast<Server*>(server);
	net::HTTPResponse response;
	net::HTTPRequest req(s->recv(client_socket));
	net::URI uri(req.start_line[1]);
	std::string path = uri.toString(false);

	if (path.find(".") == std::string::npos)
		path += "/index.html";

	if (req.headers.find("Range") == req.headers.end())
	{
		std::unique_ptr<std::string> data_ptr = load_file_data_ptr(s->getWorkDirectory() + "resources" + path);
		if (data_ptr == nullptr)
		{
			response.start_line[1] = "404";
			response.start_line[2] = "NOT FOUND";
			data_ptr = load_file_data_ptr(s->getWorkDirectory() + "resources/404/index.html");
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
		std::string file_path = s->getWorkDirectory() + "resources" + path;
		ByteRange range = get_byte_range(req.headers["Range"], get_file_size(file_path));
		std::unique_ptr<std::string> data_ptr = partially_load_file_data_ptr(file_path, range);
		if (data_ptr == nullptr)
		{
			response.start_line[1] = "404";
			response.start_line[2] = "NOT FOUND";
			data_ptr = load_file_data_ptr(s->getWorkDirectory() + "resources/404/index.html");
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

	s->send(client_socket, response.toString());
}

} //namespace app