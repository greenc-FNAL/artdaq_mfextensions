#include "MVReceiver.h"

namespace mfviewer {
  class UDPReceiver : MVReceiver {
  public:
  UDPReceiver(): partition_("0") {}
    ~UDPReceiver() {}
    const std::string& getPartition() { 
    
      return partition_; }
    void setPartition(std::string const & partition) {partition_ = partition;}
  private:
    std::string partition_;
  };
}
