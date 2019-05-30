#ifndef TCPConnect_hh
#define TCPConnect_hh
#include <arpa/inet.h>  // inet_aton
#include <netdb.h>      // gethostbyname
#include <netinet/in.h>
#include <netinet/in.h>  // struct sockaddr_in
#include <netinet/in.h>  // inet_aton
#include <stdio.h>       // printf
#include <stdlib.h>      // exit
#include <string.h>      // bzero
#include <sys/socket.h>  // socket, bind, listen, accept
#include <sys/socket.h>  // inet_aton
#include <sys/types.h>   // socket, bind, listen, accept
#include <unistd.h>      // close

#include <ifaddrs.h>
#include <linux/if_link.h>

#include <regex>
#include <string>

#include "trace.h"

// This file (TCPConnect.hh) was created by Ron Rechenmacher <ron@fnal.gov> on
// Sep 15, 2016. "TERMS AND CONDITIONS" governing this file are in the README
// or COPYING file. If you do not have such a file, one can be obtained by
// contacting Ron or Fermi Lab in Batavia IL, 60510, phone: 630-840-3000.
// $RCSfile: .emacs.gnu,v $
// rev="$Revision: 1.30 $$Date: 2016/03/01 14:27:27 $";

/**
 * \file TCPConnect.hh
 * Provides utility functions for connecting TCP sockets
 */

/**
 * \brief Convert a string hostname to a in_addr suitable for socket communication
 * \param host_in Name or IP of host to resolve
 * \param[out] addr in_addr object populated with resolved host
 * \return 0 if success, -1 if gethostbyname fails
 */
int ResolveHost(char const* host_in, in_addr& addr) {
  std::string host;
  struct hostent* hostent_sp;
  std::cmatch mm;
  //  Note: the regex expression used by regex_match has an implied ^ and $
  //        at the beginning and end respectively.
  if (regex_match(host_in, mm, std::regex("([^:]+):(\\d+)"))) {
    host = mm[1].str();
  } else if (regex_match(host_in, mm, std::regex(":{0,1}(\\d+)"))) {
    host = std::string("127.0.0.1");
  } else if (regex_match(host_in, mm, std::regex("([^:]+):{0,1}"))) {
    host = mm[1].str().c_str();
  } else {
    host = std::string("127.0.0.1");
  }
  TLOG(TLVL_INFO) << "Resolving host " << host;

  memset(&addr, 0, sizeof(addr));

  if (regex_match(host.c_str(), mm, std::regex("\\d+(\\.\\d+){3}")))
    inet_aton(host.c_str(), &addr);
  else {
    hostent_sp = gethostbyname(host.c_str());
    if (!hostent_sp) {
      perror("gethostbyname");
      return (-1);
    }
    addr = *(struct in_addr*)(hostent_sp->h_addr_list[0]);
  }
  return 0;
}

/**
 * \brief Convert an IP address to the network address of the interface sharing the subnet mask
 * \param host_in IP to resolve
 * \param[out] addr in_addr object populated with resolved host
 * \return 0 if success, -1 if gethostbyname fails, 2 if defaulted to 0.0.0.0 (No matching interfaces)
 */
int GetInterfaceForNetwork(char const* host_in, in_addr& addr) {
  std::string host;
  struct hostent* hostent_sp;
  std::cmatch mm;
  int sts = 0;
  //  Note: the regex expression used by regex_match has an implied ^ and $
  //        at the beginning and end respectively.
  if (regex_match(host_in, mm, std::regex("([^:]+):(\\d+)"))) {
    host = mm[1].str();
  } else if (regex_match(host_in, mm, std::regex(":{0,1}(\\d+)"))) {
    host = std::string("127.0.0.1");
  } else if (regex_match(host_in, mm, std::regex("([^:]+):{0,1}"))) {
    host = mm[1].str().c_str();
  } else {
    host = std::string("127.0.0.1");
  }
  TLOG(TLVL_INFO) << "Resolving ip " << host;

  memset(&addr, 0, sizeof(addr));

  if (regex_match(host.c_str(), mm, std::regex("\\d+(\\.\\d+){3}"))) {
    in_addr desired_host;
    inet_aton(host.c_str(), &desired_host);
    struct ifaddrs *ifaddr, *ifa;

    if (getifaddrs(&ifaddr) == -1) {
      perror("getifaddrs");
      return -1;
    }

    /* Walk through linked list, maintaining head pointer so we
    can free list later */

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
      if (ifa->ifa_addr == NULL) continue;

      /* For an AF_INET* interface address, display the address */

      if (ifa->ifa_addr->sa_family == AF_INET) {
        auto if_addr = (struct sockaddr_in*)ifa->ifa_addr;
        auto sa = (struct sockaddr_in*)ifa->ifa_netmask;

        TLOG(15) << "IF: " << ifa->ifa_name << " Desired: " << desired_host.s_addr
                 << " netmask: " << sa->sin_addr.s_addr << " this interface: " << if_addr->sin_addr.s_addr;

        if ((if_addr->sin_addr.s_addr & sa->sin_addr.s_addr) == (desired_host.s_addr & sa->sin_addr.s_addr)) {
          TLOG(TLVL_INFO) << "Using interface " << ifa->ifa_name;
          memcpy(&addr, &if_addr->sin_addr, sizeof(addr));
          break;
        }
      }
    }
    if (ifa == NULL) {
      TLOG(TLVL_WARNING) << "No matches for ip " << host << ", using 0.0.0.0";
      inet_aton("0.0.0.0", &addr);
      sts = 2;
    }

    freeifaddrs(ifaddr);
  } else {
    hostent_sp = gethostbyname(host.c_str());
    if (!hostent_sp) {
      perror("gethostbyname");
      return (-1);
    }
    addr = *(struct in_addr*)(hostent_sp->h_addr_list[0]);
  }
  return sts;
}

/**
 * \brief Convert a string hostname and port to a sockaddr_in suitable for socket communication
 * \param host_in Name or IP of host to resolve
 * \param dflt_port POrt to populate in output
 * \param[out] sin sockaddr_in object populated with resolved host and port
 * \return 0 if success, -1 if gethostbyname fails
 */
int ResolveHost(char const* host_in, int dflt_port, sockaddr_in& sin) {
  int port;
  std::string host;
  struct hostent* hostent_sp;
  std::cmatch mm;
  //  Note: the regex expression used by regex_match has an implied ^ and $
  //        at the beginning and end respectively.
  if (regex_match(host_in, mm, std::regex("([^:]+):(\\d+)"))) {
    host = mm[1].str();
    port = strtoul(mm[2].str().c_str(), NULL, 0);
  } else if (regex_match(host_in, mm, std::regex(":{0,1}(\\d+)"))) {
    host = std::string("127.0.0.1");
    port = strtoul(mm[1].str().c_str(), NULL, 0);
  } else if (regex_match(host_in, mm, std::regex("([^:]+):{0,1}"))) {
    host = mm[1].str().c_str();
    port = dflt_port;
  } else {
    host = std::string("127.0.0.1");
    port = dflt_port;
  }
  TLOG(TLVL_INFO) << "Resolving host " << host << ", on port " << port;

  if (host == "localhost") host = "127.0.0.1";

  memset(&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_port = htons(port);  // just a guess at an open port

  if (regex_match(host.c_str(), mm, std::regex("\\d+(\\.\\d+){3}")))
    inet_aton(host.c_str(), &sin.sin_addr);
  else {
    hostent_sp = gethostbyname(host.c_str());
    if (!hostent_sp) {
      perror("gethostbyname");
      return (-1);
    }
    sin.sin_addr = *(struct in_addr*)(hostent_sp->h_addr_list[0]);
  }
  return 0;
}

/**
 * \brief Connect to a host on a given port
 * \param host_in Name or IP of the host to connect to
 * \param dflt_port Port to connect to
 * \param flags TCP flags to use for the socket
 * \param sndbufsiz Size of the send buffer. Set to 0 for automatic send buffer management
 * \return File descriptor of connected socket.
 */
int TCPConnect(char const* host_in, int dflt_port, long flags = 0, int sndbufsiz = 0) {
  int s_fd, sts;
  struct sockaddr_in sin;

  s_fd = socket(PF_INET, SOCK_STREAM /*|SOCK_NONBLOCK*/, 0);  // man socket,man TCP(7P)

  if (s_fd == -1) {
    perror("socket error");
    return (-1);
  }

  sts = ResolveHost(host_in, dflt_port, sin);
  if (sts == -1) {
    close(s_fd);
    return -1;
  }

  sts = connect(s_fd, (struct sockaddr*)&sin, sizeof(sin));
  if (sts == -1) {
    // perror( "connect error" );
    close(s_fd);
    return (-1);
  }

  if (flags) {
    sts = fcntl(s_fd, F_SETFL, flags);
    TLOG(TLVL_TRACE) << "TCPConnect fcntl(fd=" << s_fd << ",flags=0x" << std::hex << flags << std::dec << ") =" << sts;
  }

  if (sndbufsiz > 0) {
    int len;
    socklen_t lenlen = sizeof(len);
    len = 0;
    sts = getsockopt(s_fd, SOL_SOCKET, SO_SNDBUF, &len, &lenlen);
    TLOG(TLVL_DEBUG) << "TCPConnect SNDBUF initial: " << len << " sts/errno=" << sts << "/" << errno
                     << " lenlen=" << lenlen;
    len = sndbufsiz;
    sts = setsockopt(s_fd, SOL_SOCKET, SO_SNDBUF, &len, lenlen);
    if (sts == -1) TLOG(TLVL_ERROR) << "Error with setsockopt SNDBUF " << errno;
    len = 0;
    sts = getsockopt(s_fd, SOL_SOCKET, SO_SNDBUF, &len, &lenlen);
    if (len < (sndbufsiz * 2))
      TLOG(TLVL_WARNING) << "SNDBUF " << len << " not expected (" << sndbufsiz << " sts/errno=" << sts << "/" << errno
                         << ")";
    else
      TLOG(TLVL_DEBUG) << "SNDBUF " << len << " sts/errno=" << sts << "/" << errno;
  }
  return (s_fd);
}

#endif  // TCPConnect_hh
