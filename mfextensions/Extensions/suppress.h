#ifndef MFEXTENSIONS_SUPPRESS_H
#define MFEXTENSIONS_SUPPRESS_H

#include <string>
#include <boost/regex.hpp>

typedef boost::regex   regex_t;
typedef boost::smatch  smatch_t;

class suppress
{
public:
  explicit suppress(std::string const & name);
  bool match(std::string const & name);
  void use( bool flag ) { in_use_ = flag; }

private:
  std::string name_;
  regex_t  expr_;
  smatch_t what_;
  bool     in_use_;

};

#endif
