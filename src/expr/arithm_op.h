#include "column/column_string.h"
#include "expr.h"

struct ArithmOp {
    template <typename T>
    static ColPtr Exec(const Column<T>& left, const ColPtr& right, Op op) {
        if constexpr (!std::is_arithmetic_v<T>) {
            SEND_MESSAGE("Date/Datetime arithmetic operation is not supported");
        } else {
            const Column<T>* r = static_cast<const Column<T>*>(right.get());
            std::vector<T> result(left.Size());
            for (ui64 i = 0; i < left.Size(); ++i) {
                switch (op) {
                    case Op::kAdd:
                        result[i] = left.At(i) + r->At(i);
                        break;
                    case Op::kSub:
                        result[i] = left.At(i) - r->At(i);
                        break;
                    case Op::kMul:
                        result[i] = left.At(i) * r->At(i);
                        break;
                    case Op::kDiv:
                        result[i] = left.At(i) / r->At(i);
                        break;
                    default:
                        SEND_MESSAGE("It's not an arithmetic operation");
                }
            }
            return std::make_shared<Column<T>>(std::move(result));
        }
    }

    static ColPtr Exec(const ColumnString&, const ColPtr&, Op) {
        SEND_MESSAGE("String arithmetic operation is not supported");
    }
};