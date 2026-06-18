#include "CommandLineOptions.hpp"

#include <utility>
#include <vector>
#include <string>
#include <map>


namespace app::cli
{

OptionName::OptionName():
	m_short_name(0) {}

OptionName::OptionName(char short_name):
	m_short_name(short_name) {}

OptionName::OptionName(const std::string& long_name):
	m_short_name(0),
	m_long_name(long_name) {}

OptionName::OptionName(char short_name, const std::string& long_name):
	m_short_name(short_name),
	m_long_name(long_name) {}


char OptionName::getShortName() const
{
	return m_short_name;
}

const std::string& OptionName::getLongName() const
{
	return m_long_name;
}


bool OptionName::operator==(const OptionName& other) const
{
	return ((getShortName() == other.getShortName()) &&
			(getLongName()  == other.getLongName()));

	return false;
}



bool Options::has(const OptionName& name) const
{
	for (unsigned int i = 0; i < m_flags.size(); i++)
		if (m_flags[i] == name) return true;

	for (unsigned int i = 0; i < m_one_args.size(); i++)
		if (m_one_args[i].first == name) return true;

	for (unsigned int i = 0; i < m_more_args.size(); i++)
		if (m_more_args[i].first == name) return true;

	return false;
}

const std::string* Options::getArgument(const OptionName& name) const
{
	for (unsigned int i = 0; i < m_one_args.size(); i++)
		if (m_one_args[i].first == name) return &m_one_args[i].second;
	return nullptr;
}

const std::vector<std::string>* Options::getArguments(const OptionName& name) const
{
	for (unsigned int i = 0; i < m_more_args.size(); i++)
		if (m_more_args[i].first == name) return &m_more_args[i].second;
	return nullptr;
}


const std::vector<Options::FlagOption>& Options::getFlags() const
{
	return m_flags;
}

const std::vector<Options::OneArgOption>& Options::getOneArgs() const
{
	return m_one_args;
}

const std::vector<Options::MoreArgsOption>& Options::getMoreArgs() const
{
	return m_more_args;
}


bool Options::parse(int argc, char* argv[], const std::vector<std::pair<OptionName, OptionType>>& presets)
{
	m_flags.clear();
	m_one_args.clear();
	m_more_args.clear();

	std::map<std::string, std::pair<OptionName, OptionType>> preset_map;
	for (const auto& p : presets)
	{
		const OptionName& name = p.first;
		OptionType type = p.second;

		if (name.getShortName() != 0)
			preset_map[std::string(1, name.getShortName())] = {name, type};
		if (!name.getLongName().empty())
			preset_map[name.getLongName()] = {name, type};
	}

	enum class State {NONE, COLLECT_MORE};
	State state = State::NONE;
	std::string token;
	std::vector<std::string> collected_args;
	OptionName current_option;

	for (unsigned int i = 1; i < argc; i++)
	{
		token = argv[i];

		if (state == State::COLLECT_MORE)
		{
			if (token[0] == '-')
			{
				if (collected_args.empty())
					return false;
				m_more_args.push_back({current_option, collected_args});
				collected_args.clear();
				state = State::NONE;
			}
			else
			{
				collected_args.push_back(token);
				continue;
			}
		}

		if (token[0] == '-')
		{
			if ((token.size() >= 2) && (token[1] == '-')) //--name or --name=value
			{
				std::string name_part = token.substr(2);
				std::string value;
				size_t equel_pos = name_part.find('=');
				if (equel_pos != std::string::npos)
				{
					value = name_part.substr(equel_pos + 1);
					name_part = name_part.substr(0, equel_pos);
				}
				auto it = preset_map.find(name_part);
				if (it == preset_map.end())
					return false;

				const auto& preset = it->second;
				const OptionName& opt_name = preset.first;
				OptionType        opt_type = preset.second;

				if (opt_type == OptionType::FLAG)
					m_flags.push_back(opt_name);
				else if (opt_type == OptionType::ONE_ARGUMENT)
				{
					if (!value.empty())
						m_one_args.push_back({opt_name, value});
					else
					{
						if ((i + 1) >= argc)
							return false;

						std::string next_token = argv[i + 1];
						if (next_token[0] == '-')
							return false;

						m_one_args.push_back({opt_name, next_token});
						i++;
					}
				}
				else if (opt_type == OptionType::MORE_ARGUMENTS)
				{
					state = State::COLLECT_MORE;
					current_option = opt_name;
					collected_args.clear();
					if (!value.empty())
						collected_args.push_back(value);
				}
			}
			else if (token.size() >= 2 && token[1] != '-') //-a or -abc
			{
				std::string opt_string = token.substr(1);

				if (opt_string.size() == 1) //-a
				{
					char c = opt_string[0];
					std::string key(1, c);
					auto it = preset_map.find(key);
					if (it == preset_map.end())
						return false;

					const auto& preset = it->second;
					const OptionName& opt_name = preset.first;
					OptionType        opt_type = preset.second;

					if (opt_type == OptionType::FLAG)
						m_flags.push_back(opt_name);
					else if (opt_type == OptionType::ONE_ARGUMENT)
					{
						if ((i + 1) >= argc)
							return false;

						std::string next_token = argv[i + 1];
						if (next_token[0] == '-')
							return false;

						m_one_args.push_back({opt_name, next_token});
						i++;
					}
					else if (opt_type == OptionType::MORE_ARGUMENTS)
					{
						state = State::COLLECT_MORE;
						current_option = opt_name;
						collected_args.clear();
					}
				}
				else //-abc, only flags
				{
					for (char c : opt_string)
					{
						std::string key(1, c);
						auto it = preset_map.find(key);
						if (it == preset_map.end())
							return false;

						const auto& preset = it->second;
						if (preset.second != OptionType::FLAG)
							return false;
						m_flags.push_back(preset.first);
					}
				}
			}
			else //token == "-"
				return false;
		}
		else //token starts on '-', but not State::COLLECT_MORE
			return false;
	}

	if (state == State::COLLECT_MORE)
	{
		if (collected_args.empty())
			return false;
		m_more_args.push_back({current_option, collected_args});
	}

	return true;
}

} //namespace app::cli
