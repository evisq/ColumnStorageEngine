#pragma once

#include <cstring>

#include "column/column.h"
#include "util/assert.h"

class ColumnString : public ColumnBase {
public:
    ColumnString() = default;
    ColumnString(std::vector<char> data, std::vector<ui64> offsets)
        : data_(std::move(data)), offsets_(std::move(offsets)) {
        ASSERT_WITH_MESSAGE(!offsets_.empty(),
                            "Offsets must have at least 1 element");
    }
    ColType GetType() const override { return ColType::kString; }

    ui64 Size() const override { return offsets_.size() - 1; }

    std::string_view At(ui64 i) const {
        ASSERT_WITH_MESSAGE(i < Size(),
                            "The row index is outside the column size");
        return {data_.data() + offsets_[i], offsets_[i + 1] - offsets_[i]};
    }

    static ColPtr FromBytes(const char *src, ui64 byte_size, ui64 num_rows) {
        const ui64 off_bytes = (num_rows + 1) * sizeof(ui64);
        ASSERT_WITH_MESSAGE(off_bytes <= byte_size, "Byte_size is too small");
        const ui64 data_size = byte_size - off_bytes;
        std::vector<char> data(src, src + data_size);
        std::vector<ui64> offsets(num_rows + 1);
        std::memcpy(offsets.data(), src + data_size, off_bytes);
        return std::make_shared<ColumnString>(std::move(data),
                                              std::move(offsets));
    }

private:
    std::vector<char> data_;
    std::vector<ui64> offsets_;
};