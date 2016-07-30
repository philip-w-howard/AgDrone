#pragma once

#include <list>

#include "datablock.h"

class DataList
{
    public:
        DataList();
        virtual ~DataList();

        bool Insert(int start, int size, void *data);
        bool GetHole(int *start, int *size, int start_addr = -1);
        bool GetUnsentBlock(int *start, int *size, void **data);
        void ListAllBlocks();
        bool DataIsComplete();
        void SetDataComplete();
    protected:
        std::list<DataBlock> m_data;
        bool m_DataComplete;
};
