#include "cetlib/PluginTypeDeducer.h"
#include "fhiclcpp/ParameterSet.h"
#include "fhiclcpp/types/ConfigurationTable.h"
#include "fhiclcpp/types/Sequence.h"
#include "fhiclcpp/types/TableFragment.h"

#include "messagefacility/MessageService/ELdestination.h"
#include "messagefacility/Utilities/ELseverityLevel.h"
#if MESSAGEFACILITY_HEX_VERSION < 0x20201  //  v2_02_01 is s67
#include "messagefacility/MessageService/MessageDrop.h"
#else
#include "messagefacility/MessageLogger/MessageLogger.h"
#endif
#include "messagefacility/Utilities/exception.h"

// C/C++ includes
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <netinet/in.h>
#include <algorithm>
#include <atomic>
#include <boost/thread.hpp>
#include <memory>
#include <mutex>
#include <random>

#include <QtCore/QString>
#include "cetlib/compiler_macros.h"
#include "mfextensions/Destinations/detail/curl_send_message.h"

namespace mfplugins {
using mf::ELseverityLevel;
using mf::ErrorObj;
using mf::service::ELdestination;

/// <summary>
/// SMTP Message Facility destination plugin (Using libcurl)
/// </summary>
class ELSMTP : public ELdestination {
 public:
  /**
   * \brief Configuration parameters for ELSMTP
   */
  struct Config {
    /// Array of strings
    using strings_t = fhicl::Sequence<std::string>::default_type;
    /// ELDestination common config parameters
    fhicl::TableFragment<ELdestination::Config> elDestConfig;
    /// "host" (Default: "smtp.fnal.gov"): SMTP Server hostname
    fhicl::Atom<std::string> host =
        fhicl::Atom<std::string>{fhicl::Name{"host"}, fhicl::Comment{"SMTP Server hostname"}, "smtp.fnal.gov"};
    /// "port" (Default: 25): SMTP Server port
    fhicl::Atom<int> port = fhicl::Atom<int>{fhicl::Name{"port"}, fhicl::Comment{"SMTP Server port"}, 25};
    /// "to_addresses" (Default: {}): The list of email addresses that SMTP mfPlugin should sent to
    fhicl::Sequence<std::string> to = fhicl::Sequence<std::string>{
        fhicl::Name{"to_addresses"}, fhicl::Comment{"The list of email addresses that SMTP mfPlugin should sent to"},
        strings_t{}};
    /// "from_address" (REQUIRED): Source email address
    fhicl::Atom<std::string> from =
        fhicl::Atom<std::string>{fhicl::Name{"from_address"}, fhicl::Comment{"Source email address"}};
    /// "subject" (Default: "MessageFacility SMTP Message Digest"): Subject of the email message
    fhicl::Atom<std::string> subject = fhicl::Atom<std::string>{
        fhicl::Name{"subject"}, fhicl::Comment{"Subject of the email message"}, "MessageFacility SMTP Message Digest"};
    /// "message_header" (Default: ""): String to preface messages with in email body
    fhicl::Atom<std::string> messageHeader = fhicl::Atom<std::string>{
        fhicl::Name{"message_header"}, fhicl::Comment{"String to preface messages with in email body"}, ""};
    /// "use_smtps" (Default: false): Use SMTPS protocol
    fhicl::Atom<bool> useSmtps =
        fhicl::Atom<bool>{fhicl::Name{"use_smtps"}, fhicl::Comment{"Use SMTPS protocol"}, false};
    /// "smtp_username" (Default: ""): Username for SMTP server
    fhicl::Atom<std::string> user =
        fhicl::Atom<std::string>{fhicl::Name{"smtp_username"}, fhicl::Comment{"Username for SMTP server"}, ""};
    /// "smtp_password" (Default: ""): Password for SMTP server
    fhicl::Atom<std::string> pw =
        fhicl::Atom<std::string>{fhicl::Name{"smtp_password"}, fhicl::Comment{"Password for SMTP server"}, ""};
    /// "verify_host_ssl_certificate" (Default: true): Whether to run full SSL verify on SMTP server in SMTPS mode
    fhicl::Atom<bool> verifyCert =
        fhicl::Atom<bool>{fhicl::Name{"verify_host_ssl_certificate"},
                          fhicl::Comment{"Whether to run full SSL verify on SMTP server in SMTPS mode"}, true};
    /// "email_send_interval_seconds" (Default: 15): Only send email every N seconds
    fhicl::Atom<size_t> sendInterval = fhicl::Atom<size_t>{fhicl::Name{"email_send_interval_seconds"},
                                                           fhicl::Comment{"Only send email every N seconds"}, 15};
  };
  /// Used for ParameterSet validation
  using Parameters = fhicl::WrappedTable<Config>;

 public:
  /// <summary>
  /// ELSMTP Constructor
  /// </summary>
  /// <param name="pset">ParameterSet used to configure ELSMTP</param>
  ELSMTP(Parameters const& pset);

  ~ELSMTP() {
    abort_sleep_ = true;
    while (sending_thread_active_) usleep(1000);
  }

  /**
   * \brief Serialize a MessageFacility message to the output
   * \param o Stringstream object containing message data
   * \param msg MessageFacility object containing header information
   */
  virtual void routePayload(const std::ostringstream& o, const ErrorObj& msg) override;

 private:
  void send_message_();
  std::string generateMessageId_() const;
  std::string dateTimeNow_();
  std::string to_html(std::string msgString, const ErrorObj& msg);

  std::string smtp_host_;
  int port_;
  std::vector<std::string> to_;
  std::string from_;
  std::string subject_;
  std::string message_prefix_;

  // Message information
  long pid_;
  std::string hostname_;
  std::string hostaddr_;
  std::string app_;

  bool use_ssl_;
  std::string username_;
  std::string password_;
  bool ssl_verify_host_cert_;

  std::atomic<bool> sending_thread_active_;
  std::atomic<bool> abort_sleep_;
  size_t send_interval_s_;
  mutable std::mutex message_mutex_;
  std::ostringstream message_contents_;
};

// END DECLARATION
//======================================================================
// BEGIN IMPLEMENTATION

//======================================================================
// ELSMTP c'tor
//======================================================================
ELSMTP::ELSMTP(Parameters const& pset)
    : ELdestination(pset().elDestConfig()),
      smtp_host_(pset().host()),
      port_(pset().port()),
      to_(pset().to()),
      from_(pset().from()),
      subject_(pset().subject()),
      message_prefix_(pset().messageHeader()),
      pid_(static_cast<long>(getpid())),
      use_ssl_(pset().useSmtps()),
      username_(pset().user()),
      password_(pset().pw()),
      ssl_verify_host_cert_(pset().verifyCert()),
      sending_thread_active_(false),
      abort_sleep_(false),
      send_interval_s_(pset().sendInterval()) {
  // hostname
  char hostname_c[1024];
  hostname_ = (gethostname(hostname_c, 1023) == 0) ? hostname_c : "Unkonwn Host";

  // host ip address
  hostent* host = nullptr;
  host = gethostbyname(hostname_c);

  if (host != nullptr) {
    // ip address from hostname if the entry exists in /etc/hosts
    char* ip = inet_ntoa(*(struct in_addr*)host->h_addr);
    hostaddr_ = ip;
  } else {
    // enumerate all network interfaces
    struct ifaddrs* ifAddrStruct = nullptr;
    struct ifaddrs* ifa = nullptr;
    void* tmpAddrPtr = nullptr;

    if (getifaddrs(&ifAddrStruct)) {
      // failed to get addr struct
      hostaddr_ = "127.0.0.1";
    } else {
      // iterate through all interfaces
      for (ifa = ifAddrStruct; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr->sa_family == AF_INET) {
          // a valid IPv4 addres
          tmpAddrPtr = &((struct sockaddr_in*)ifa->ifa_addr)->sin_addr;
          char addressBuffer[INET_ADDRSTRLEN];
          inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
          hostaddr_ = addressBuffer;
        }

        else if (ifa->ifa_addr->sa_family == AF_INET6) {
          // a valid IPv6 address
          tmpAddrPtr = &((struct sockaddr_in6*)ifa->ifa_addr)->sin6_addr;
          char addressBuffer[INET6_ADDRSTRLEN];
          inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);
          hostaddr_ = addressBuffer;
        }

        // find first non-local address
        if (!hostaddr_.empty() && hostaddr_.compare("127.0.0.1") && hostaddr_.compare("::1")) break;
      }

      if (hostaddr_.empty())  // failed to find anything
        hostaddr_ = "127.0.0.1";
    }
  }

  // get process name from '/proc/pid/exe'
  std::string exe;
  std::ostringstream pid_ostr;
  pid_ostr << "/proc/" << pid_ << "/exe";
  exe = realpath(pid_ostr.str().c_str(), NULL);

  size_t end = exe.find('\0');
  size_t start = exe.find_last_of('/', end);

  app_ = exe.substr(start + 1, end - start - 1);
}

std::string ELSMTP::to_html(std::string msgString, const ErrorObj& msg) {
  auto sevid = msg.xid().severity().getLevel();

  QString text_ = QString("<font color=");

  switch (sevid) {
    case mf::ELseverityLevel::ELsev_success:
    case mf::ELseverityLevel::ELsev_zeroSeverity:
    case mf::ELseverityLevel::ELsev_unspecified:
      text_ += QString("#505050>");
      break;

    case mf::ELseverityLevel::ELsev_info:
      text_ += QString("#008000>");
      break;

    case mf::ELseverityLevel::ELsev_warning:
      text_ += QString("#E08000>");
      break;

    case mf::ELseverityLevel::ELsev_error:
    case mf::ELseverityLevel::ELsev_severe:
    case mf::ELseverityLevel::ELsev_highestSeverity:
      text_ += QString("#FF0000>");
      break;

    default:
      break;
  }

  // std::cout << "qt_mf_msg.cc:" << msg.message() << std::endl;
  text_ += QString("<pre>") + QString(msgString.c_str()).toHtmlEscaped()  // + "<br>"
           + QString("</pre>");

  text_ += QString("</font>");
  return text_.toStdString();
}

//======================================================================
// Message router ( overriddes ELdestination::routePayload )
//======================================================================
void ELSMTP::routePayload(const std::ostringstream& oss, const ErrorObj& msg) {
  std::lock_guard<std::mutex> lk(message_mutex_);
  message_contents_ << to_html(oss.str(), msg);

  if (!sending_thread_active_) {
    sending_thread_active_ = true;
    boost::thread t([=] { send_message_(); });
    t.detach();
  }
}

void ELSMTP::send_message_() {
  size_t slept = 0;
  while (!abort_sleep_ && slept < send_interval_s_ * 1000000) {
    usleep(10000);
    slept += 10000;
  }

  std::string payload;
  {
    std::lock_guard<std::mutex> lk(message_mutex_);
    payload = message_contents_.str();
    message_contents_.str("");
  }
  std::string destination = (use_ssl_ ? "smtps://" : "smtp://") + smtp_host_ + ":" + std::to_string(port_);

  std::vector<const char*> to;
  to.reserve(to_.size());
  std::string toString;
  for (size_t i = 0; i < to_.size(); ++i) {
    to.push_back(to_[i].c_str());
    toString += to_[i];
    if (i < to_.size() - 1) {
      toString += ", ";
    }
  }

  std::ostringstream headers_builder;

  headers_builder << "Date: " << dateTimeNow_() << "\r\n";
  headers_builder << "To: " << toString << "\r\n";
  headers_builder << "From: " << from_ << "\r\n";
  headers_builder << "Message-ID:  <" + generateMessageId_() + "@" + from_.substr(from_.find('@') + 1) + ">\r\n";
  headers_builder << "Subject: " << subject_ << " @ " << dateTimeNow_() << " from PID " << getpid() << "\r\n";
  headers_builder << "Content-Type: text/html; charset=\"UTF-8\"\r\n";
  headers_builder << "\r\n";

  std::string headers = headers_builder.str();
  std::ostringstream message_builder;
  message_builder << headers << "<html><body><p>" << message_prefix_ << "</p>" << payload << "</body></html>";
  std::string payloadWithHeaders = message_builder.str();

  if (use_ssl_) {
    send_message_ssl(destination.c_str(), &to[0], to_.size(), from_.c_str(), payloadWithHeaders.c_str(),
                     payloadWithHeaders.size(), username_.c_str(), password_.c_str(), !ssl_verify_host_cert_);
  } else {
    send_message(destination.c_str(), &to[0], to_.size(), from_.c_str(), payloadWithHeaders.c_str(),
                 payloadWithHeaders.size());
  }
  sending_thread_active_ = false;
}

// https://codereview.stackexchange.com/questions/140409/sending-email-using-libcurl-follow-up/140562
std::string ELSMTP::generateMessageId_() const {
  const size_t MESSAGE_ID_LEN = 37;
  tm t;
  time_t tt;
  time(&tt);
  gmtime_r(&tt, &t);

  std::string ret;
  ret.resize(MESSAGE_ID_LEN);
  size_t datelen = std::strftime(&ret[0], MESSAGE_ID_LEN, "%Y%m%d%H%M%S", &t);
  static const std::string alphaNum{
      "0123456789"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "abcdefghijklmnopqrstuvwxyz"};
  std::mt19937 gen;
  std::uniform_int_distribution<> dis(0, alphaNum.length() - 1);
  std::generate_n(ret.begin() + datelen, MESSAGE_ID_LEN - datelen, [&]() { return alphaNum[dis(gen)]; });
  return ret;
}

std::string ELSMTP::dateTimeNow_() {
  const int RFC5322_TIME_LEN = 32;

  std::string ret;
  ret.resize(RFC5322_TIME_LEN);

  time_t tt;
  tm tv, *t = &tv;
  tt = time(&tt);
  localtime_r(&tt, t);

  strftime(&ret[0], RFC5322_TIME_LEN, "%a, %d %b %Y %H:%M:%S %z", t);

  return ret;
}
}  // end namespace mfplugins

//======================================================================
//
// makePlugin function
//
//======================================================================

#ifndef EXTERN_C_FUNC_DECLARE_START
#define EXTERN_C_FUNC_DECLARE_START extern "C" {
#endif

EXTERN_C_FUNC_DECLARE_START
auto makePlugin(const std::string&, const fhicl::ParameterSet& pset) {
  return std::make_unique<mfplugins::ELSMTP>(pset);
}
}

DEFINE_BASIC_PLUGINTYPE_FUNC(mf::service::ELdestination)
