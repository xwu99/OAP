#include "precompile/array.h"

#include <arrow/array.h>

namespace sparkcolumnarplugin {
namespace precompile {

Array::Array(const std::shared_ptr<arrow::Array>& in) : cache_(in) {
  offset_ = in->offset();
  length_ = in->length();
  null_count_ = in->null_count();
  null_bitmap_data_ = in->null_bitmap_data();
  raw_value_ = in->data()->buffers[1]->mutable_data();
}

BooleanArray::BooleanArray(const std::shared_ptr<arrow::Array>& in) : cache_(in) {
  offset_ = in->offset();
  length_ = in->length();
  null_count_ = in->null_count();
  null_bitmap_data_ = in->null_bitmap_data();
  raw_value_ = in->data()->buffers[1]->data();
}

#define TYPED_NUMERIC_ARRAY_IMPL(TYPENAME, TYPE)                             \
  TYPENAME::TYPENAME(const std::shared_ptr<arrow::Array>& in) : cache_(in) { \
    offset_ = in->offset();                                                  \
    length_ = in->length();                                                  \
    null_count_ = in->null_count();                                          \
    null_bitmap_data_ = in->null_bitmap_data();                              \
    auto typed_in = std::dynamic_pointer_cast<arrow::TYPENAME>(in);          \
    raw_value_ = typed_in->raw_values();                                     \
  }

TYPED_NUMERIC_ARRAY_IMPL(Int32Array, int32_t)
TYPED_NUMERIC_ARRAY_IMPL(Int64Array, int64_t)
TYPED_NUMERIC_ARRAY_IMPL(UInt32Array, uint32_t)
TYPED_NUMERIC_ARRAY_IMPL(UInt64Array, uint64_t)
TYPED_NUMERIC_ARRAY_IMPL(FloatArray, float)
TYPED_NUMERIC_ARRAY_IMPL(DoubleArray, double)
TYPED_NUMERIC_ARRAY_IMPL(Date32Array, int32_t)
TYPED_NUMERIC_ARRAY_IMPL(FixedSizeBinaryArray, uint8_t)
#undef TYPED_NUMERIC_ARRAY_IMPL

#define TYPED_BINARY_ARRAY_IMPL(TYPENAME, TYPE)                              \
  TYPENAME::TYPENAME(const std::shared_ptr<arrow::Array>& in) : cache_(in) { \
    offset_ = in->offset();                                                  \
    length_ = in->length();                                                  \
    null_count_ = in->null_count();                                          \
    null_bitmap_data_ = in->null_bitmap_data();                              \
    auto typed_in = std::dynamic_pointer_cast<arrow::TYPENAME>(in);          \
    auto value_offsets = typed_in->value_offsets();                          \
    auto value_data = typed_in->value_data();                                \
    raw_value_ = value_data == NULLPTR ? NULLPTR : value_data->data();       \
    raw_value_offsets_ =                                                     \
        value_offsets == NULLPTR                                             \
            ? NULLPTR                                                        \
            : reinterpret_cast<const offset_type*>(value_offsets->data());   \
  }

TYPED_BINARY_ARRAY_IMPL(StringArray, std::string)
#undef TYPED_ARROW_ARRAY_IMPL

arrow::Status MakeFixedSizeBinaryArray(
    const std::shared_ptr<arrow::FixedSizeBinaryType>& type, int64_t length,
    const std::shared_ptr<arrow::Buffer>& buffer,
    std::shared_ptr<FixedSizeBinaryArray>* out) {
  auto arrow_out = std::make_shared<arrow::FixedSizeBinaryArray>(type, length, buffer);
  *out = std::make_shared<FixedSizeBinaryArray>(arrow_out);
  return arrow::Status::OK();
}
}  // namespace precompile
}  // namespace sparkcolumnarplugin