/**
 * @file MVReceiver.cc
 *
 * This is part of the artdaq Framework, copyright 2023.
 * Licensing/copyright details are in the LICENSE file that you should have
 * received with this code.
 */
#include "mfextensions/Receivers/MVReceiver.hh"
#include "moc_MVReceiver.cpp"  // NOLINT

mfviewer::MVReceiver::MVReceiver(fhicl::ParameterSet const& /*pset*/)
    : stopRequested_(false)
{
	std::cout << "MVReceiver Constructor" << std::endl;
}
