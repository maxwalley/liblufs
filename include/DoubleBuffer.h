/*
This code was inspired by the farbot RealtimeObject implementation. See https://github.com/hogliux/farbot for a more developed approach
*/

#pragma once

#include <atomic>
#include <array>
#include <cassert>

namespace LUFS
{

template<typename T>
class DoubleBuffer
{
public:
    DoubleBuffer(const T& initialVal)  : buffers{initialVal, initialVal}, realtimeCopy{initialVal} {}
    
    ~DoubleBuffer()
    {
        //Busy bit should not be set
        assert((control.load() & BusyBit) == 0);
    }
    
    T& realtimeAquire()
    {
        return realtimeCopy;
    }
    
    void realtimeRelease()
    {
        //Set the busy bit and get the write index
        const int writeIndex = control.fetch_or(BusyBit) & IndexBit;
        buffers[writeIndex] = realtimeCopy;
        
        //Unset the busy bit, set the new data bit
        control.store((writeIndex & IndexBit) | NewDataBit);
    }
    
    const T& nonRealtimeRead() const
    {
        int current = control.load();
        
        if(current & NewDataBit)
        {
            int newValue;
            
            //CAS loop to spin while busy bit is set, then set the new buffer to write to
            do
            {
                current &= ~BusyBit;
                newValue = (current ^ IndexBit) & IndexBit;
            }
            while(!control.compare_exchange_weak(current, newValue));
            
            current = newValue;
        }
        
        //Return the last buffer that was written to
        return buffers[(current & IndexBit) ^ 0x1];
    }
    
    //This is not realtime safe with the other calls
    void reset(const T& newValue)
    {
        assert((control.load() & BusyBit) == 0);
        
        buffers[0] = newValue;
        buffers[1] = newValue;
        realtimeCopy = newValue;
    }
    
private:
    enum
    {
        IndexBit = (1 << 0),
        NewDataBit = (1 << 1),
        BusyBit = (1 << 2)
    };
    
    mutable std::atomic<int> control;
    
    std::array<T, 2> buffers;
    T realtimeCopy;
};

}
