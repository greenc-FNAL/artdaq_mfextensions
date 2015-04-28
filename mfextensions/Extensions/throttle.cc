

#include "mfextensions/Extensions/throttle.hh"

throttle::throttle( std::string const & name, int limit, long timespan )
: name_     ( name )
, expr_     ( regex_t(name) )
, what_     (  )
, limit_    ( limit )
, timespan_ ( timespan )
, last_window_start_ ( 0 )
, count_    ( 0 )
, in_use_   ( true )
{

}

bool throttle::reach_limit(std::string const & name, timeval tm)
{
  if( !in_use_ ) return false;

  if( !boost::regex_match( name, what_, expr_ ) )
    return false;

  if( limit_==0 ) return true;       // suppress
  else if( limit_<0 ) return false;  // no limit

  if( timespan_<=0 )
  {
    // only display first "limit_" messages
    ++count_;
    return count_>limit_ ? true : false;
  }

  long sec = tm.tv_sec;
  if( sec - last_window_start_ > timespan_ )
  {
    last_window_start_ = sec;
    count_ = 1;
  }
  else
  {
    ++count_;
  }

  return count_ > limit_  ? true : false;
}



