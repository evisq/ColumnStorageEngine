#pragma once
#include <utility>
#include <vector>

#include "op.h"
#include "reader/reader_ISQ.h"

class ScanOp : public Op {
public:
    ScanOp(std::string isq_path, std::vector<ui64> indices)
        : reader_(isq_path, std::move(indices)) {}

    bool HasNext() override { return reader_.HasNext(); }
    Batch Next() override { return reader_.Next(); }

private:
    ReaderISQ reader_;
};