#pragma once

#include <atomic>
#include <thread>

class SpinLock
{
public:
    SpinLock() {}

    void lock()
    {
        while(!try_lock())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds{100});
        }
    }

    bool try_lock()
    {
        bool expected = false;

        return lockedFlag.compare_exchange_strong(expected, true, std::memory_order_seq_cst);
    }

    void unlock()
    {
        lockedFlag.store(false);
    }

private:
    std::atomic<bool> lockedFlag = false;
};