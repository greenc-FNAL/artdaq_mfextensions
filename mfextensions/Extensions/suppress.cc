/**
 * @file suppress.cc
 *
 * This is part of the artdaq Framework, copyright 2023.
 * Licensing/copyright details are in the LICENSE file that you should have
 * received with this code.
 */
#include "mfextensions/Extensions/suppress.hh"

suppress::suppress(std::string const& name)
    : name_(name), expr_(regex_t(name)), in_use_(true) {}

bool suppress::match(std::string const& name)
{
	if (!in_use_)
	{
		return false;
	}
	return boost::regex_match(name, what_, expr_);
}
