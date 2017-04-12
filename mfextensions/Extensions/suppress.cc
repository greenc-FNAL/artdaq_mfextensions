#include "mfextensions/Extensions/suppress.hh"

suppress::suppress(std::string const& name)
	: name_(name)
	, expr_(regex_t(name))
	, what_()
	, in_use_(true) {}

bool suppress::match(std::string const& name)
{
	if (!in_use_) return false;
	return boost::regex_match(name, what_, expr_);
}
