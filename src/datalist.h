#pragma once

#include <list>

#include "datablock.h"

class DataList
{
    public:
        DataList();
        virtual ~DataList();

        void Insert(int start, int size, void *data);
        bool GetHole(int *start, int *size);
        bool GetUnsentBlock(int *start, int *size, void **data);
        bool DataIsComplete();
        void SetDataComplete();
    protected:
        std::list<DataBlock> m_data;
        bool m_DataComplete;
};
