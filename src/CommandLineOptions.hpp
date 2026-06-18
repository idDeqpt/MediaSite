#ifndef COMMAND_LINE_OPTIONS_CLASS_HEADER
#define COMMAND_LINE_OPTIONS_CLASS_HEADER

#include <utility>
#include <vector>
#include <string>


namespace app::cli
{
	enum OptionType
	{
		FLAG,
		ONE_ARGUMENT,
		MORE_ARGUMENTS
	};

	class OptionName
	{
	public:
		OptionName();
		OptionName(char short_name);
		OptionName(const std::string& long_name);
		OptionName(char short_name, const std::string& long_name);

		char getShortName() const;
		const std::string& getLongName() const;

		bool operator==(const OptionName& other) const;

	protected:
		char m_short_name;
		std::string m_long_name;
	};

	class Options
	{
	public:
		using FlagOption     = OptionName;
		using OneArgOption   = std::pair<OptionName, std::string>;
		using MoreArgsOption = std::pair<OptionName, std::vector<std::string>>;

		bool has(const OptionName& name) const;

		const std::string*              getArgument(const OptionName& name) const;
		const std::vector<std::string>* getArguments(const OptionName& name) const;

		const std::vector<FlagOption>&     getFlags()    const;
		const std::vector<OneArgOption>&   getOneArgs()  const;
		const std::vector<MoreArgsOption>& getMoreArgs() const;

		bool parse(int argc, char* argv[], const std::vector<std::pair<OptionName, OptionType>>& presets);

	protected:
		std::vector<FlagOption>     m_flags;
		std::vector<OneArgOption>   m_one_args;
		std::vector<MoreArgsOption> m_more_args;
	};
}

#endif //COMMAND_LINE_OPTIONS_CLASS_HEADER