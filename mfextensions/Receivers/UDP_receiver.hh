#ifndef MFVIEWER_RECEIVERS_UDP_RECEIVER_HH
#define MFVIEWER_RECEIVERS_UDP_RECEIVER_HH

#include "mfextensions/Receivers/MVReceiver.hh"

#include <messagefacility/MessageLogger/MessageLogger.h>
#ifdef NO_MF_UTILITIES
#include <messagefacility/MessageLogger/MessageFacilityMsg.h>
#else
#include <messagefacility/Utilities/MessageFacilityMsg.h>
#endif

#include <boost/asio.hpp>
using boost::asio::ip::udp;

namespace mfviewer
{
	class UDPReceiver : public MVReceiver
	{
		Q_OBJECT
	public:
		explicit UDPReceiver(fhicl::ParameterSet pset);

		virtual ~UDPReceiver();

		//Reciever Method
		void run() override;

		// Message Parser
		mf::MessageFacilityMsg read_msg(std::string input);

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
