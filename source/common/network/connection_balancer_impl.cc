#include <iostream>

#include "source/common/network/connection_balancer_impl.h"

namespace Envoy {
namespace Network {

void ExactConnectionBalancerImpl::registerHandler(BalancedConnectionHandler& handler) {
  absl::MutexLock lock(&lock_);
  handlers_.push_back(&handler);
}

void ExactConnectionBalancerImpl::unregisterHandler(BalancedConnectionHandler& handler) {
  absl::MutexLock lock(&lock_);
  // This could be made more efficient in various ways, but the number of listeners is generally
  // small and this is a rare operation so we can start with this and optimize later if this
  // becomes a perf bottleneck.
  handlers_.erase(std::find(handlers_.begin(), handlers_.end(), &handler));
}

BalancedConnectionHandler&
ExactConnectionBalancerImpl::pickTargetHandler(BalancedConnectionHandler&, int connSeq) {
  BalancedConnectionHandler* min_connection_handler = nullptr;
  {
    absl::MutexLock lock(&lock_);
    if (connSeq != 0) {
      std::cerr << "handlers_.size(): " << handlers_.size() << std::endl;
      int workerCount = handlers_.size();
      uint32_t targetWorkerIndex = connSeq % workerCount;
      std::cerr << "targetWorkerIndex: " << targetWorkerIndex << std::endl;
      for (BalancedConnectionHandler* handler : handlers_) {
        std::cerr << "handler worker index : " << handler->workerIndex() << std::endl;
        if (handler->workerIndex() == targetWorkerIndex) {
          min_connection_handler = handler;
          break;
        }
      }
      std::cerr << "picking handler " << min_connection_handler << " based on sequence " << connSeq << std::endl;
      
    } else {
      for (BalancedConnectionHandler* handler : handlers_) {
        if (min_connection_handler == nullptr ||
            handler->numConnections() < min_connection_handler->numConnections()) {
          min_connection_handler = handler;
        }
      }
    }

    min_connection_handler->incNumConnections();
  }

  return *min_connection_handler;
}

} // namespace Network
} // namespace Envoy
