#pragma once

#include <assert.h>
#include <stdlib.h>
#include <string.h>

class DataBlock
{
    public:
        DataBlock()
        {
            m_start = 0;
            m_size = 0;
            m_sent = false;
        }

        DataBlock(int start, int size, void *data)
        {
            m_start = start;
            m_size = size;
            m_sent = false;
        }

        virtual ~DataBlock()
        {
        }

        bool Append(int size, void *data)
        {
            if (m_size + size > M_SIZE) return false;

            memcpy(m_data+m_size, data, size);
            m_size += size;

            return true;
        }

        int Start() { return m_start; }
        int Size() { return m_size; }
        void *Data() { return m_data; }
        bool IsSent() { return m_sent; }
        void MarkSent() { m_sent = true; }

    protected:
        static const int M_SIZE = 4069;

        int m_start;
        int m_size;
        char m_data[M_SIZE];
        bool m_sent;
};
