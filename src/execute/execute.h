#pragma once

#include <memory>
#include <vector>

#include "aggr/aggr.h"
#include "column/column_int_dt.h"
#include "expr/expr.h"
#include "reader/reader_ISQ.h"

// Прогоняет все батчи через фильтр и агрегации.
// filter  — выражение WHERE (nullptr = все строки)
// aggs    — список агрегаций
// agg_cols— индекс колонки батча для каждой агрегации
inline void Execute(ReaderISQ& reader,
                    const ExprPtr& filter,
                    const std::vector<std::shared_ptr<Aggr>>& aggs,
                    const std::vector<ui64>& agg_cols) {
    while (reader.HasNext()) {
        Batch batch = reader.Next();

        ColPtr mask_col = filter
            ? filter->Eval(batch)
            : std::make_shared<Column<ui8>>(std::vector<ui8>(batch.cnt_rows, 1));

        const auto& mask = *static_cast<Column<ui8>*>(mask_col.get());

        for (ui64 i = 0; i < aggs.size(); ++i)
            aggs[i]->Update(batch[agg_cols[i]], mask);
    }
}
