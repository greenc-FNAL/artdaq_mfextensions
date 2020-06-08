#include "MVReceiver.hh"
#include "moc_MVReceiver.cpp"
mfviewer::MVReceiver::MVReceiver(fhicl::ParameterSet const& /*pset*/)
    : stopRequested_(false)
{
	std::cout << "MVReceiver Constructor" << std::endl;
}
