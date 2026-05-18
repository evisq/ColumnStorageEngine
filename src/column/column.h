#pragma once

#include <memory>

#include "col_util.h"
#include "util/alias.h"

class ColumnBase {
public:
    virtual ~ColumnBase() = default;
    virtual ColType GetType() const = 0;
    virtual ui64 Size() const = 0;
};

using ColPtr = std::shared_ptr<ColumnBase>;
