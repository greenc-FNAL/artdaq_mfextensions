#include "cetlib/PluginTypeDeducer.h"
#include "fhiclcpp/ParameterSet.h"

#include "messagefacility/MessageService/ELdestination.h"
#include "messagefacility/Utilities/ELseverityLevel.h"
#if MESSAGEFACILITY_HEX_VERSION < 0x20002 // v2_00_02 is s50, pre v2_00_02 is s48
# include "messagefacility/MessageService/ELcontextSupplier.h"
# include "messagefacility/MessageLogger/MessageDrop.h"
#else
# include "messagefacility/MessageService/MessageDrop.h"
#endif
#include "messagefacility/Utilities/exception.h"

// C/C++ includes
#include <memory>
#include <algorithm>
#include <atomic>
#include <thread>
#include <mutex>
#include <random>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <netinet/in.h>

#include <QtCore/QString>
#include "mfextensions/Destinations/detail/curl_send_message.h"

namespace mfplugins
{
	using mf::service::ELdestination;
	using mf::ELseverityLevel;
	using mf::ErrorObj;
# if MESSAGEFACILITY_HEX_VERSION < 0x20002 // v2_00_02 is s50, pre v2_00_02 is s48
	using mf::service::ELcontextSupplier;
# endif

	//======================================================================
	//
	// SMTP destination plugin (Using libcurl)
	//
	//======================================================================

	class ELSMTP : public ELdestination
	{
	public:

		ELSMTP(const fhicl::ParameterSet& pset);

		virtual void routePayload(const std::ostringstream&, const ErrorObj& msg
# if MESSAGEFACILITY_HEX_VERSION < 0x20002 // v2_00_02 is s50, pre v2_00_02 is s48
								  , const ELcontextSupplier&
# endif
		) override;

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

	ELSMTP::ELSMTP(const fhicl::ParameterSet& pset)
		: ELdestination(pset)
		, smtp_host_(pset.get<std::string>("host", "smtp.fnal.gov"))
		, port_(pset.get<int>("port", 25))
		, to_(pset.get<std::vector<std::string>>("to_addresses"))
		, from_(pset.get<std::string>("from_address"))
		, subject_(pset.get<std::string>("subject", "MessageFacility SMTP Message Digest"))
		, message_prefix_(pset.get<std::string>("message_header",""))
		, pid_(static_cast<long>(getpid()))
		, use_ssl_(pset.get<bool>("use_smtps", false))
		, username_(pset.get<std::string>("smtp_username", ""))
		, password_(pset.get<std::string>("smtp_password", ""))
		, ssl_verify_host_cert_(pset.get<bool>("verify_host_ssl_certificate", true))
		, sending_thread_active_(false)
		, send_interval_s_(pset.get<size_t>("email_send_interval_seconds", 15))
	{
		// hostname
		char hostname_c[1024];
		hostname_ = (gethostname(hostname_c, 1023) == 0) ? hostname_c : "Unkonwn Host";

		// host ip address
		hostent* host = nullptr;
		host = gethostbyname(hostname_c);

		if (host != nullptr)
		{
			// ip address from hostname if the entry exists in /etc/hosts
			char* ip = inet_ntoa(*(struct in_addr *)host->h_addr);
			hostaddr_ = ip;
		}
		else
		{
			// enumerate all network interfaces
			struct ifaddrs* ifAddrStruct = nullptr;
			struct ifaddrs* ifa = nullptr;
			void* tmpAddrPtr = nullptr;

			if (getifaddrs(&ifAddrStruct))
			{
				// failed to get addr struct
				hostaddr_ = "127.0.0.1";
			}
			else
			{
				// iterate through all interfaces
				for (ifa = ifAddrStruct; ifa != nullptr; ifa = ifa->ifa_next)
				{
					if (ifa->ifa_addr->sa_family == AF_INET)
					{
						// a valid IPv4 addres
						tmpAddrPtr = &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
						char addressBuffer[INET_ADDRSTRLEN];
						inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
						hostaddr_ = addressBuffer;
					}

					else if (ifa->ifa_addr->sa_family == AF_INET6)
					{
						// a valid IPv6 address
						tmpAddrPtr = &((struct sockaddr_in6 *)ifa->ifa_addr)->sin6_addr;
						char addressBuffer[INET6_ADDRSTRLEN];
						inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);
						hostaddr_ = addressBuffer;
					}

					// find first non-local address
					if (!hostaddr_.empty()
						&& hostaddr_.compare("127.0.0.1")
						&& hostaddr_.compare("::1"))
						break;
				}

				if (hostaddr_.empty()) // failed to find anything
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


	std::string ELSMTP::to_html(std::string msgString, const ErrorObj& msg)
	{
		mf::ELseverityLevel severity(msg.xid().severity());
		int sevid = severity.getLevel();

		QString text_ = QString("<font color=");

		switch (sevid)
		{
#  if MESSAGEFACILITY_HEX_VERSION < 0x20002 // v2_00_02 is s50, pre v2_00_02 is s48
		case mf::ELseverityLevel::ELsev_incidental:
#  endif
		case mf::ELseverityLevel::ELsev_success:
		case mf::ELseverityLevel::ELsev_zeroSeverity:
		case mf::ELseverityLevel::ELsev_unspecified:
			text_ += QString("#505050>");
			break;

		case mf::ELseverityLevel::ELsev_info:
			text_ += QString("#008000>");
			break;

		case mf::ELseverityLevel::ELsev_warning:
#  if MESSAGEFACILITY_HEX_VERSION < 0x20002 // v2_00_02 is s50, pre v2_00_02 is s48
		case mf::ELseverityLevel::ELsev_warning2:
#  endif
			text_ += QString("#E08000>");
			break;

		case mf::ELseverityLevel::ELsev_error:
#  if MESSAGEFACILITY_HEX_VERSION < 0x20002 // v2_00_02 is s50, pre v2_00_02 is s48
		case mf::ELseverityLevel::ELsev_error2:
		case mf::ELseverityLevel::ELsev_next:
		case mf::ELseverityLevel::ELsev_severe2:
		case mf::ELseverityLevel::ELsev_abort:
		case mf::ELseverityLevel::ELsev_fatal:
#  endif
		case mf::ELseverityLevel::ELsev_severe:
		case mf::ELseverityLevel::ELsev_highestSeverity:
			text_ += QString("#FF0000>");
			break;

		default: break;
		}

		//std::cout << "qt_mf_msg.cc:" << msg.message() << std::endl;
		text_ += QString("<pre>")
			+ QString(msgString.c_str()).toHtmlEscaped() // + "<br>"
			+ QString("</pre>");

		text_ += QString("</font>");
		return text_.toStdString();
	}

	//======================================================================
	// Message router ( overriddes ELdestination::routePayload )
	//======================================================================
	void ELSMTP::routePayload(const std::ostringstream& oss
#if MESSAGEFACILITY_HEX_VERSION < 0x20002 // v2_00_02 is s50, pre v2_00_02 is s48
							  , const ErrorObj& msg, ELcontextSupplier const&
#else
							  , const ErrorObj& msg
#endif
	)
	{
		std::unique_lock<std::mutex>(message_mutex_);
		message_contents_ << to_html(oss.str(), msg);

		if (!sending_thread_active_)
		{
			sending_thread_active_ = true;
			std::thread t([=] { send_message_(); });
			t.detach();
		}
	}

	void ELSMTP::send_message_()
	{
		sleep(send_interval_s_);

		std::string payload;
		{
			std::unique_lock<std::mutex> lk(message_mutex_);
			payload = message_contents_.str();
			message_contents_.str("");
		}
		std::string destination = "smtp://" + smtp_host_ + ":" + std::to_string(port_);


		std::vector<const char*> to;
		to.reserve(to_.size());
		std::string toString;
		for (size_t i = 0; i < to_.size(); ++i)
		{
			to.push_back(to_[i].c_str());
			toString += to_[i];
			if (i < to_.size() - 1)
			{
				toString += ", ";
			}
		}

		std::ostringstream headers_builder;

		headers_builder << "Date: " << dateTimeNow_() << "\r\n";
		headers_builder << "To: " << toString << "\r\n";
		headers_builder << "From: " << from_ << "\r\n";
		headers_builder << "Message-ID:  <" + generateMessageId_() + "@" + from_.substr(from_.find('@') + 1) + ">\r\n";
		headers_builder << "Subject: " << subject_ << "\r\n";
		headers_builder << "Content-Type: text/html; charset=\"UTF-8\"\r\n";
		headers_builder << "\r\n";

		std::string headers = headers_builder.str();
		std::ostringstream message_builder;
		message_builder << headers << "<html><body><p>" << message_prefix_ << "</p>" << payload << "</body></html>";
		std::string payloadWithHeaders = message_builder.str();


		if (use_ssl_)
		{
			send_message_ssl(destination.c_str(), &to[0], to_.size(), from_.c_str(), payloadWithHeaders.c_str(), payloadWithHeaders.size(), username_.c_str(), password_.c_str(), !ssl_verify_host_cert_);
		}
		else
		{
			send_message(destination.c_str(), &to[0], to_.size(), from_.c_str(), payloadWithHeaders.c_str(), payloadWithHeaders.size());
		}
		sending_thread_active_ = false;
	}

	//https://codereview.stackexchange.com/questions/140409/sending-email-using-libcurl-follow-up/140562
	std::string ELSMTP::generateMessageId_() const
	{
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
			"abcdefghijklmnopqrstuvwxyz" };
		std::mt19937 gen;
		std::uniform_int_distribution<> dis(0, alphaNum.length() - 1);
		std::generate_n(ret.begin() + datelen, MESSAGE_ID_LEN - datelen, [&]() { return alphaNum[dis(gen)]; });
		return ret;
	}

	std::string ELSMTP::dateTimeNow_()
	{
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
} // end namespace mfplugins

  //======================================================================
  //
  // makePlugin function
  //
  //======================================================================

extern "C"
{
	auto makePlugin(const std::string&,
					const fhicl::ParameterSet& pset)
	{
		return std::make_unique<mfplugins::ELSMTP>(pset);
	}
}

DEFINE_BASIC_PLUGINTYPE_FUNC(mf::service::ELdestination)
