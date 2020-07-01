
#if 0
#include "ErrorHandler/MessageAnalyzer/ma_utils.h"

using namespace novadaq::errorhandler;

int novadaq::errorhandler::and_op (int i, int j)
{ 
  if (i==j)           return i;

  if (i==-2 || j==-2) return -2;
  if (i==-1)          return j;
  if (j==-1)          return i;

  return -2;
}

std::string novadaq::errorhandler::trim_hostname(std::string const & host)
{
  size_t pos = host.find('.');
  if (pos==std::string::npos) return host;
  else                        return host.substr(0, pos);
}

node_type_t novadaq::errorhandler::get_source_from_msg(std::string & src, qt_mf_msg const & msg)
{
  std::string host = trim_hostname(msg.hostname());

  if (  (host.find("dcm")!=std::string::npos) )
  {
    src  = host; return DCM;
  }
  else if (host.find("novadaq-ctrl-farm")!=std::string::npos)
  {
    src  = host; return BufferNode;
  }
  else
  {
    src  = msg.application(); return MainComponent;
  }
}

ma_cond_domain novadaq::errorhandler::domain_and(ma_cond_domain const & d1, ma_cond_domain const & d2)
{
  return ma_cond_domain( and_op(d1.first, d2.first)
                       , and_op(d1.second, d2.second));
}

void domain_and( ma_cond_domain & d1, ma_cond_domain const & d2 )
{
  d1.first  = and_op(d1.first, d2.first);
  d1.second = and_op(d1.second, d2.second);
}

ma_cond_domain novadaq::errorhandler::domain_and(ma_cond_domains const & d)
{
  if (d.empty())   return ma_cond_domain(-2,-2);

  ma_cond_domain d_out = d.front();
  ma_cond_domains::const_iterator it = d.begin();
  while(++it!=d.end()) domain_and(d_out, *it);

  return d_out;
}

void novadaq::errorhandler::domain_and(ma_cond_domains & d)
{
  if (d.empty()) 
  {
    d.push_back(ma_cond_domain(-2,-2));
    return;
  }

  ma_cond_domains::const_iterator it = d.begin();
  while(++it!=d.end())  domain_and(d.front(), *it);
}


#endif










