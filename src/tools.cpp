#include "tools.hpp"

#include <Network/HTTP.hpp>

#include <fstream>
#include <sstream>
#include <string>
#include <memory>


namespace tools
{

bool is_request_complete(const std::string& data, int& content_length)
{
	if (content_length == -1)
	{
		unsigned int pos = data.find("Content-Length:");
		if (pos == std::string::npos)
			content_length = 0;
		else
		{
			pos += 15;
			unsigned int end = data.find("\r\n", pos);
			if (end != std::string::npos)
				content_length = std::stoi(data.substr(pos, end - pos));
		}
	}
	if (content_length == 0)
		return data.find("\r\n\r\n") != std::string::npos;
	else
	{
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


void full_file_load_handler(net::HTTPResponse& response, const std::string& resources_dir, const std::string& file_path)
{
	std::unique_ptr<std::string> data_ptr = load_file_data_ptr(resources_dir + file_path);
	if (data_ptr == nullptr)
	{
		response.start_line[1] = "404";
		response.start_line[2] = "NOT FOUND";
		data_ptr = load_file_data_ptr(resources_dir + "404/index.html");
	}
	else
	{
		response.start_line[1] = "200";
		response.start_line[2] = "OK";
	}
	response.body = *data_ptr;
}

void range_file_load_handler(net::HTTPRequest& request, net::HTTPResponse& response, const std::string& resources_dir, const std::string& file_path)
{
	std::string full_file_path = resources_dir + file_path;
	ByteRange range = get_byte_range(request.headers["Range"], get_file_size(full_file_path));
	std::unique_ptr<std::string> data_ptr = partially_load_file_data_ptr(full_file_path, range);
	if (data_ptr == nullptr)
	{
		response.start_line[1] = "404";
		response.start_line[2] = "NOT FOUND";
		data_ptr = load_file_data_ptr(resources_dir + "404/index.html");
	}

	response.start_line[1] = "206";
	response.start_line[2] = "PARTIAL CONTENT";
	response.headers["Connection"] = "keep-alive";
	response.headers["Cache-Control"] = "no-cache";
	response.headers["Accept-Ranges"] = "bytes";
	response.headers["Content-Range"] = "bytes " + std::to_string(range.start) + "-" + std::to_string(range.end) + "/" + std::to_string(range.total);
	response.body = *data_ptr;
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

} //namespace tools