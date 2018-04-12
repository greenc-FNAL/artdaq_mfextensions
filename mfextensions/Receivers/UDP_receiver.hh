#ifndef MFVIEWER_RECEIVERS_UDP_RECEIVER_HH
#define MFVIEWER_RECEIVERS_UDP_RECEIVER_HH

#include "mfextensions/Receivers/MVReceiver.hh"

#include "messagefacility/MessageLogger/MessageLogger.h"

#include <boost/asio.hpp>
using boost::asio::ip::udp;

namespace mfviewer
{
	/// <summary>
	/// Receive messages through a UDP socket. Expects the syslog format provided by UDP_mfPlugin (ELUDP)
	/// </summary>
	class UDPReceiver : public MVReceiver
	{
		Q_OBJECT
	public:
		explicit UDPReceiver(fhicl::ParameterSet pset);

		virtual ~UDPReceiver();

		//Reciever Method
		void run() override;

		// Message Parser
		qt_mf_msg read_msg(std::string input);

		static bool validate_packet(std::string input);

	private:
		int port_;
		boost::asio::io_service io_service_;
		udp::socket socket_;
		char buffer_[0x10000];
		bool debug_;
	};
}

#endif
