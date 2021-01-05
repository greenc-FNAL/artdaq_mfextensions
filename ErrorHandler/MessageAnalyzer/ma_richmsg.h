#ifndef ERRORHANDLER_MA_RICHMSG_H
#define ERRORHANDLER_MA_RICHMSG_H

#include "ErrorHandler/MessageAnalyzer/ma_types.h"

#include <vector>

namespace novadaq {
namespace errorhandler {

class ma_rule;

class ma_richmsg
{
public:
	ma_richmsg();
	ma_richmsg(std::string const& s, ma_rule const* parent);

	~ma_richmsg() {}

	void init(ma_rule const* parent, std::string const& s);

	const std::string& plain_message() const;
	std::string message() const;

private:
	ma_rule const* rule;
	std::string plain_msg;
	std::string stripped_msg;

	std::vector<size_t> insert_pos;
	std::vector<cond_arg_t> symbols;
};

}  // end of namespace errorhandler
}  // end of namespace novadaq

#endif
