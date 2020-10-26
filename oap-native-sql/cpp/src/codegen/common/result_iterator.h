/*
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <arrow/status.h>
#include <arrow/type_fwd.h>

class ResultIteratorBase {
 public:
  virtual bool HasNext() { return false; }
  virtual std::string ToString() { return ""; }
};

template <typename T>
class ResultIterator : public ResultIteratorBase {
 public:
  virtual bool HasNext() override { return false; }
  virtual arrow::Status Next(std::shared_ptr<T>* out) {
    return arrow::Status::NotImplemented("ResultIterator abstract Next()");
  }
  virtual arrow::Status Process(
      const std::vector<std::shared_ptr<arrow::Array>>& in, std::shared_ptr<T>* out,
      const std::shared_ptr<arrow::Array>& selection = nullptr) {
    return arrow::Status::NotImplemented("ResultIterator abstract Process()");
  }
  virtual arrow::Status Process(
      const std::vector<std::shared_ptr<arrow::Array>>& in,
      const std::vector<std::shared_ptr<arrow::Array>>& projected_in,
      std::shared_ptr<T>* out, const std::shared_ptr<arrow::Array>& selection = nullptr) {
    return arrow::Status::NotImplemented("ResultIterator abstract Process()");
  }
  virtual arrow::Status ProcessAndCacheOne(
      const std::vector<std::shared_ptr<arrow::Array>>& in,
      const std::shared_ptr<arrow::Array>& selection = nullptr) {
    return arrow::Status::NotImplemented("ResultIterator abstract ProcessAndCacheOne()");
  }
  virtual arrow::Status SetDependencies(
      const std::vector<std::shared_ptr<ResultIteratorBase>>&) {
    return arrow::Status::NotImplemented("ResultIterator abstract SetDependencies()");
  }
  virtual arrow::Status GetResult(std::shared_ptr<arrow::RecordBatch>* out) {
    return arrow::Status::NotImplemented("ResultIterator abstract GetResult()");
  }
  virtual std::string ToString() override { return ""; }
};
