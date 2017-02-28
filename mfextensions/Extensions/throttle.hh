#ifndef artdaq_mfextensions_extensions_throttle_hh
#define artdaq_mfextensions_extensions_throttle_hh

#include <string>

#include <sys/time.h>

#include <boost/regex.hpp>

typedef boost::regex   regex_t;
typedef boost::smatch  smatch_t;

class throttle
{
public:
  throttle(std::string const & name, int limit, long timespan);
  bool reach_limit(std::string const & name, timeval tm);
  void use( bool flag ) { in_use_ = flag; }

private:
  std::string name_;
  regex_t  expr_;
  smatch_t what_;
  int      limit_;
  long     timespan_;
  long     last_window_start_;
  int      count_;
  bool     in_use_;
};

#endif //artdaq_mfextensions_extensions_throttle_hh