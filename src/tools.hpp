#ifndef TOOLS_FUNCTIONS_HEADER
#define TOOLS_FUNCTIONS_HEADER

#include <string>
#include <memory>

namespace net
{
	class HTTPRequest;
	class HTTPResponse;
}

namespace tools
{
	struct ByteRange
	{
		unsigned int start;
		unsigned int end;
		unsigned int total;
	};

	bool is_request_complete(const std::string& data, int& content_length);

	std::string  get_content_type(const std::string& path);
	unsigned int get_file_size(const std::string& path);
	ByteRange    get_byte_range(const std::string& range_string, unsigned int file_size);

	void full_file_load_handler(net::HTTPResponse& response, const std::string& resources_dir, const std::string& file_path);
	void range_file_load_handler(net::HTTPRequest& request, net::HTTPResponse& response, const std::string& resources_dir, const std::string& file_path);

	std::unique_ptr<std::string> load_file_data_ptr(const std::string& path);
	std::unique_ptr<std::string> partially_load_file_data_ptr(const std::string& path, ByteRange byte_range);
}

#endif //TOOLS_FUNCTIONS_HEADER