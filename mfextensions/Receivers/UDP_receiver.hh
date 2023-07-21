/**
 * @file UDP_receiver.hh
 *
 * This is part of the artdaq Framework, copyright 2023.
 * Licensing/copyright details are in the LICENSE file that you should have
 * received with this code.
 */
#ifndef MFEXTENSIONS_RECEIVERS_UDP_RECEIVER_HH_
#define MFEXTENSIONS_RECEIVERS_UDP_RECEIVER_HH_

#include "mfextensions/Receivers/MVReceiver.hh"

#include "messagefacility/MessageLogger/MessageLogger.h"

#include <list>
#include <string>

namespace mfviewer {
/// <summary>
/// Receive messages through a UDP socket. Expects the syslog format provided by UDP_mfPlugin (ELUDP)
/// </summary>
class UDPReceiver : public MVReceiver
{
	Q_OBJECT
public:
	/// <summary>
	/// UDPReceiver Constructor
	/// </summary>
	/// <param name="pset">ParameterSet to use to configure the receiver</param>
	explicit UDPReceiver(fhicl::ParameterSet const& pset);

	/// <summary>
	/// Destructor -- Close socket
	/// </summary>
	virtual ~UDPReceiver();

	/// <summary>
	/// Receiver method. Receive messages and emit NewMessage signal
	/// </summary>
	void run() override;

	/// <summary>
	/// Parse incoming message
	/// </summary>
	/// <param name="input">String to parse</param>
	/// <returns>qt_mf_msg object containing message data</returns>
	msg_ptr_t read_msg(std::string const& input);

	/// <summary>
	/// Run simple validation tests on message
	/// </summary>
	/// <param name="input">String to validate</param>
	/// <returns>True if message contains "MF" marker and at least one "|" delimeter</returns>
	static bool validate_packet(std::string const& input);

private:
	UDPReceiver(UDPReceiver const&) = delete;
	UDPReceiver(UDPReceiver&&) = delete;
	UDPReceiver& operator=(UDPReceiver const&) = delete;
	UDPReceiver& operator=(UDPReceiver&&) = delete;

	void setupMessageListener_();

	int message_port_;
	std::string message_addr_;
	bool multicast_enable_;
	std::string multicast_out_addr_;
	int message_socket_;

	std::list<std::string> tokenize_(std::string const& input);
};
}  // namespace mfviewer

#endif  // MFEXTENSIONS_RECEIVERS_UDP_RECEIVER_HH_
