/*
  Pms5003Async.h
  
  Specific to ESP (espsoftwareserial.h)
*/

#ifndef _PMS5003Async_H
#define _PMS5003Async_H

#include "Pms5003.h"

class Pms5003Async_Base : public Pms5003
{
public:
    Pms5003Async_Base(Stream &out) : Pms5003(out)
    {
    }
    Pms5003Async_Base(const Pms5003Async_Base &) = delete;
    Pms5003Async_Base &operator=(const Pms5003Async_Base &) = delete;

    virtual void perform_work()
    {
        perform_work_query_data_auto();
    }

    bool query_data_auto_async(int n, PMSDATA_t *data_tbl);

    void on_query_data_auto_completed(Delegate<void(int n), void *> handler)
    {
        query_data_auto_handler = handler;
    }

protected:
    Stream &_get_out() { return _out; }
    virtual void onReceive(Delegate<void(int available), void *> handler) = 0;
    void perform_work_query_data_auto();
    Delegate<void(int n), void *> query_data_auto_handler;

    enum QueryDataAutoState
    {
        QDA_OFF,
        QDA_WAITCOLLECT,
        QDA_RAMPUP,
        QDA_COLLECTING
    };

    QueryDataAutoState query_data_auto_state = QDA_OFF;
    uint32_t query_data_auto_start;
    uint32_t query_data_auto_deadline;
    int query_data_auto_n = 0;
    PMSDATA_t *query_data_auto_data_ptr = 0;
    int query_data_auto_collected = 0;
}; // end cloass Pms5003Async_Base

template <class S>
class Pms5003Async : public Pms5003Async_Base
{
    static_assert(std::is_base_of<Stream, S>::value, "S must derive from Stream");

public:
    Pms5003Async(S &out) : Pms5003Async_Base(out)
    {
    }

    void perform_work() override
    {
        _get_out().perform_work();
        Pms5003Async_Base::perform_work();
    }

protected:
    S &_get_out() { return static_cast<S &>(_out); }
    void onReceive(Delegate<void(int available), void *> handler) override
    {
        _get_out().onReceive(handler);
    }
}; // end class Pms5300Async

template <>
class Pms5003Async<HardwareSerial> : public Pms5003Async_Base
{
public:
    // construction
    Pms5003Async(HardwareSerial &out) : Pms5003Async_Base(out)
    {
    }

    void perform_work() override
    {
        if (receiveHandler)
        {
            int avail = _get_out().available();
            if (avail)
            {
                receiveHandler(avail);
            }
        }
        Pms5003Async_Base::perform_work();
    }

protected:
    HardwareSerial &_get_out() { return static_cast<HardwareSerial &>(_out); }
    void onReceive(Delegate<void(int available), void *> handler) override
    {
        receiveHandler = handler;
    }
    Delegate<void(int available), void *> receiveHandler;
}; // end class Pms5003Sync

#endif
