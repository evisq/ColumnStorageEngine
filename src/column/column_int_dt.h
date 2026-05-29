#pragma once

#include <cstring>

#include "column/column.h"
#include "util/assert.h"

template <typename T>
class Column : public ColumnBase {
public:
    Column() = default;
    explicit Column(std::vector<T> data) : data_(std::move(data)) {}

    ColType GetType() const override;

    ui64 Size() const override { return data_.size(); }

    const T &At(ui64 ind) const { return data_[ind]; }

    const std::vector<T> &Data() const { return data_; }

    static ColPtr FromBytes(const char *src, ui64 num_rows) {
        std::vector<T> result(num_rows);
        std::memcpy(result.data(), src, num_rows * sizeof(T));
        return std::make_shared<Column<T>>(std::move(result));
    }

private:
    std::vector<T> data_;
};

template <typename T>
ColType Column<T>::GetType() const {
    ASSERT_WITH_MESSAGE(sizeof(T) == 0, "Unsupported type of Column");
    return ColType::kInt8;
}
template <>
inline ColType Column<i8>::GetType() const {
    return ColType::kInt8;
}
template <>
inline ColType Column<i16>::GetType() const {
    return ColType::kInt16;
}
template <>
inline ColType Column<i32>::GetType() const {
    return ColType::kInt32;
}
template <>
inline ColType Column<i64>::GetType() const {
    return ColType::kInt64;
}
template <>
inline ColType Column<IsqDate>::GetType() const {
    return ColType::kDate;
}
template <>
inline ColType Column<IsqDatetime>::GetType() const {
    return ColType::kDatetime;
}
