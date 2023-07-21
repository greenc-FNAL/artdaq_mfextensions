/**
 * @file suppress.hh
 *
 * This is part of the artdaq Framework, copyright 2023.
 * Licensing/copyright details are in the LICENSE file that you should have
 * received with this code.
 */
#ifndef MFEXTENSIONS_EXTENSIONS_SUPPRESS_HH_
#define MFEXTENSIONS_EXTENSIONS_SUPPRESS_HH_

#include <boost/regex.hpp>
#include <string>

typedef boost::regex regex_t;
typedef boost::smatch smatch_t;

/// <summary>
/// Suppress messages based on a regular expression
/// </summary>
class suppress
{
public:
	/// <summary>
	/// Construct a suppression using the given name for regex matching
	/// </summary>
	/// <param name="name">Name to suppress</param>
	explicit suppress(std::string const& name);

	/// <summary>
	/// Check if the name matches this suppression
	/// </summary>
	/// <param name="name">Name to check</param>
	/// <returns>True if name should be suppressed</returns>
	bool match(std::string const& name);

	/// <summary>
	/// Set whether the suppression is active
	/// </summary>
	/// <param name="flag">Whether the suppression should be active</param>
	void use(bool flag) { in_use_ = flag; }

private:
	std::string name_;
	regex_t expr_;
	smatch_t what_;
	bool in_use_;
};

#endif  // MFEXTENSIONS_EXTENSIONS_SUPPRESS_HH_
