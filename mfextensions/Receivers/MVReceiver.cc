#include "MVReceiver.hh"
#include "moc_MVReceiver.cpp"
mfviewer::MVReceiver::MVReceiver(fhicl::ParameterSet): stopRequested_(false) { std::cout << "MVReceiver Constructor" << std::endl; }
