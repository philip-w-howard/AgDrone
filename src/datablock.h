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
            m_data = NULL;
            m_data_extent = 0;
            m_sent = false;
        }

        DataBlock(int start, int size, void *data)
        {
            m_start = start;
            m_size = size;
            m_data = (char *)malloc(M_EXTEND_SIZE);
            m_data_extent = M_EXTEND_SIZE;
            m_sent = false;
        }

        virtual ~DataBlock()
        {
            if (m_data != NULL) free(m_data);
        }

        bool Append(int size, void *data)
        {
            if (m_size + size > M_MAX_SIZE) return false;

            if (m_size + size > m_data_extent)
            {
                if (size < M_EXTEND_SIZE)
                    m_data = (char *)realloc(m_data, 
                            m_data_extent + M_EXTEND_SIZE);
                else
                    m_data = (char *)realloc(m_data, m_data_extent + size);
                assert(m_data != NULL);
            }

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
        static const int M_EXTEND_SIZE = 4069;
        static const int M_MAX_SIZE = 4069;

        int m_start;
        int m_size;
        char *m_data;
        int m_data_extent;
        bool m_sent;
};
