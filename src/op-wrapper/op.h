#pragma once
#include <memory>

#include "column/batch.h"

class Op {
public:
    virtual bool HasNext() = 0;
    virtual Batch Next() = 0;
    virtual ~Op() = default;
};

using OpPtr = std::shared_ptr<Op>;