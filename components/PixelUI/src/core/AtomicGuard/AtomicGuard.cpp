#include "PixelUI.h"
#include <atomic>

class AtomicGuard {
public:
    AtomicGuard(std::atomic<bool>& flag) : _flag(flag) {
        _flag.store(true, std::memory_order_relaxed);
    }
    ~AtomicGuard() {
        _flag.store(false, std::memory_order_relaxed);
    }
private:
    std::atomic<bool>& _flag;
};