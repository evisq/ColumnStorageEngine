#pragma once

#include <cstring>
#include <fstream>
#include <numeric>
#include <vector>

#include "column/batch.h"
#include "column/col_util.h"
#include "column/column.h"
#include "column/deserialize.h"
#include "util/alias.h"
#include "util/assert.h"
#include "util/stream.h"

class ReaderISQ {
public:
    explicit ReaderISQ(std::string_view path) {
        file_ = std::ifstream(std::string(path), std::ios::binary);
        ASSERT_WITH_MESSAGE(file_.is_open(),
                            "ISQ file is not opening: " + std::string(path));
        file_.seekg(-8, std::ios::end);
        ui64 data_pos = Get<ui64>(file_);
        file_.seekg(data_pos);

        ui64 batch_cnt = Get<ui64>(file_);
        cnt_cols_ = Get<ui64>(file_);
        batch_size_ = Get<ui64>(file_);

        batch_idx_offsets_.resize(batch_cnt);
        for (ui64 i = 0; i < batch_cnt; ++i) {
            batch_idx_offsets_[i] = Get<ui64>(file_);
        }
        col_inds_.resize(cnt_cols_);
        std::iota(col_inds_.begin(), col_inds_.end(), 0);

        scheme_.resize(cnt_cols_);
        for (ui64 i = 0; i < cnt_cols_; ++i) {
            scheme_[i].type = Get<ColType>(file_);
        }

        std::vector<ui64> offset(cnt_cols_ + 1);
        for (ui64 i = 0; i < cnt_cols_ + 1; ++i) {
            offset[i] = Get<ui64>(file_);
        }
        for (ui64 i = 0; i < cnt_cols_; ++i) {
            const size_t str_size = offset[i + 1] - offset[i];
            scheme_[i].name.resize(str_size);
            file_.read(scheme_[i].name.data(), str_size);
        }
    }

    ReaderISQ(std::string_view path, std::vector<ui64> ind) {
        file_ = std::ifstream(std::string(path), std::ios::binary);
        ASSERT_WITH_MESSAGE(file_.is_open(),
                            "ISQ file is not opening: " + std::string(path));
        file_.seekg(-8, std::ios::end);
        ui64 data_pos = Get<ui64>(file_);
        file_.seekg(data_pos);

        ui64 batch_cnt = Get<ui64>(file_);
        cnt_cols_ = Get<ui64>(file_);
        batch_size_ = Get<ui64>(file_);

        batch_idx_offsets_.resize(batch_cnt);
        for (ui64 i = 0; i < batch_cnt; ++i) {
            batch_idx_offsets_[i] = Get<ui64>(file_);
        }
        col_inds_ = ind;

        scheme_.resize(ind.size());
        ui64 pos = static_cast<ui64>(file_.tellg());
        for (ui64 i = 0; i < ind.size(); ++i) {
            file_.seekg(pos + ind[i]);
            scheme_[i].type = Get<ColType>(file_);
        }
        file_.seekg(pos + cnt_cols_);

        std::vector<ui64> offset(cnt_cols_ + 1);
        for (ui64 i = 0; i < cnt_cols_ + 1; ++i) {
            offset[i] = Get<ui64>(file_);
        }
        pos = static_cast<ui64>(file_.tellg());
        for (ui64 i = 0; i < ind.size(); ++i) {
            const size_t str_size = offset[ind[i] + 1] - offset[ind[i]];
            file_.seekg(pos + offset[ind[i]]);
            scheme_[i].name.resize(str_size);
            file_.read(scheme_[i].name.data(), str_size);
        }
    }

    ~ReaderISQ() = default;

    bool HasNext() const { return batch_id_ < batch_idx_offsets_.size(); }

    Batch Next() {
        ASSERT_WITH_MESSAGE(HasNext(), "There is no next batch");
        ui64 batch_pos = batch_idx_offsets_[batch_id_++];

        std::vector<ColPtr> table_;
        file_.seekg(batch_pos + cnt_cols_ * sizeof(ui64));
        ui64 cnt_rows = Get<ui64>(file_);
        std::vector<char> buf;
        for (ui64 i = 0; i < col_inds_.size(); ++i) {
            file_.seekg(batch_pos + col_inds_[i] * sizeof(ui64));
            ui64 col_pos = Get<ui64>(file_);
            ui64 col_pos_next =
                (col_inds_[i] == cnt_cols_ - 1 ? batch_pos : Get<ui64>(file_));
            buf.resize(col_pos_next - col_pos);
            file_.seekg(col_pos);
            file_.read(buf.data(), buf.size());
            table_.push_back(DeserializeColumn(buf.data(), buf.size(),
                                               scheme_[i].type, cnt_rows));
        }
        return {std::move(table_), cnt_rows};
    }

    void Reset() { batch_id_ = 0; }

    const std::vector<ColScheme>& GetScheme() const { return scheme_; }

    ui64 NumBatches() { return batch_idx_offsets_.size(); }

private:
    std::ifstream file_;
    ui64 cnt_cols_;
    ui64 batch_size_;
    std::vector<ui64> batch_idx_offsets_;
    std::vector<ui64> col_inds_;

    std::vector<ColScheme> scheme_;

    ui64 batch_id_ = 0;
};