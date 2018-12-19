#ifndef artdaq_mfextensions_extensions_throttle_hh
#define artdaq_mfextensions_extensions_throttle_hh

#include <string>

#include <sys/time.h>

#include <boost/regex.hpp>

typedef boost::regex regex_t;
typedef boost::smatch smatch_t;

/// <summary>
/// Throttle messages based on name and time limits.
/// Separate from MessageFacility limiting.
/// </summary>
class throttle {
 public:
  /// <summary>
  /// Throttle messages using a regular expression if they occurr above a certain frequency
  /// </summary>
  /// <param name="name">Regular expression to match messages</param>
  /// <param name="limit">Number of messages before throttling is enabled</param>
  /// <param name="timespan">Time limit for throttling</param>
  throttle(std::string const& name, int limit, long timespan);

  /// <summary>
  /// Determine whether the name has reached the throttling limit
  /// </summary>
  /// <param name="name">Name to check against regular expression</param>
  /// <param name="tm">Time of message</param>
  /// <returns>Whether the message should be throttled</returns>
  bool reach_limit(std::string const& name, timeval tm);

  /// <summary>
  /// Enable or disable this throttle
  /// </summary>
  /// <param name="flag">Whether the throttle should be enabled</param>
  void use(bool flag) { in_use_ = flag; }

 private:
  std::string name_;
  regex_t expr_;
  smatch_t what_;
  int limit_;
  long timespan_;
  long last_window_start_;
  int count_;
  bool in_use_;
};

#endif  // artdaq_mfextensions_extensions_throttle_hh
