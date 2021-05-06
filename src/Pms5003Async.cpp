/*
  Pms5003Async.cpp
  
  Specific to ESP (espsoftwareserial.h)
*/

#include "Pms5003Async.h"

bool Pms5003Async_Base::query_data_auto_async(int n, PMSDATA_t *data_tbl)
{
    if (QDA_OFF != query_data_auto_state)
        return false;
    query_data_auto_n = n;
    query_data_auto_data_ptr = data_tbl;
    query_data_auto_collected = 0;

    query_data_auto_state = QDA_WAITCOLLECT;

    onReceive([this](int avail) {
        int estimatedMsgCnt = avail / 10;
        PMSDATA_t data;
        int dataAutoCnt = 0;

        while (estimatedMsgCnt--)
            if (query_data_auto(data))
            {
                ++dataAutoCnt;
            }

        // estimate 1s cutting into rampup per data_auto msg
        if (dataAutoCnt > 0)
        {
            --dataAutoCnt;

            query_data_auto_state = QDA_RAMPUP;
            query_data_auto_start = millis();
            query_data_auto_deadline = (rampup_s - dataAutoCnt) * 1000UL;

            onReceive([this](int avail) {
                unsigned long deadlineExpired = millis() - query_data_auto_start;
                if (deadlineExpired < query_data_auto_deadline)
                {
                    _get_out().flush();
                    return;
                }
                PMSDATA_t data;
                // discard estimated msgs prior to deadline expiration
                while (avail >= 10 && deadlineExpired - query_data_auto_deadline >= 1000UL)
                {
                    avail -= 10;
                    if (query_data_auto(data))
                        deadlineExpired -= 1000UL;
                }

                query_data_auto_state = QDA_COLLECTING;
                query_data_auto_start = millis();
                query_data_auto_deadline = 1000UL / 4UL * rampup_s;
                onReceive([this](int avail) {
                    PMSDATA_t data;
                    while (avail >= 10 && query_data_auto_collected < query_data_auto_n)
                    {
                        avail -= 10;
                        if (query_data_auto(data))
                        {
                            *query_data_auto_data_ptr++ = data;
                            ++query_data_auto_collected;
                        }
                        query_data_auto_start = millis();
                    }
                    if (query_data_auto_collected >= query_data_auto_n)
                    {
                        if (query_data_auto_handler)
                            query_data_auto_handler(query_data_auto_collected);
                        query_data_auto_handler = nullptr;
                        query_data_auto_state = QDA_OFF;
                        query_data_auto_data_ptr = 0;
                        query_data_auto_collected = 0;
                        onReceive(nullptr);
                    }
                });
            });
        }
    });
    return true;
}

void Pms5003Async_Base::perform_work_query_data_auto()
{
    if (QDA_COLLECTING == query_data_auto_state &&
        millis() - query_data_auto_start > query_data_auto_deadline)
    {
        if (query_data_auto_handler)
            query_data_auto_handler(query_data_auto_collected);
        query_data_auto_handler = nullptr;
        query_data_auto_state = QDA_OFF;
        query_data_auto_data_ptr = 0;
        query_data_auto_collected = 0;
        onReceive(nullptr);
    }
}