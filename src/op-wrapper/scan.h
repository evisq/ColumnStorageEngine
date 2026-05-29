#pragma once
#include <utility>
#include <vector>

#include "op-wrapper/op.h"
#include "reader/isq_reader.h"

class ScanOp : public Op {
public:
    ScanOp(const std::string& isq_path, std::vector<ui64>&& indices)
        : reader_(isq_path, std::move(indices)) {}

    explicit ScanOp(const std::string& isq_path) : reader_(isq_path) {}

    bool HasNext() override { return reader_.HasNext(); }

    Batch Next() override { return reader_.Next(); }

private:
    IsqReader reader_;
};
