/* Copyright 2018 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/
#ifndef TENSORFLOW_CORE_COMMON_RUNTIME_TEST_COLLECTIVE_EXECUTOR_MGR_H_
#define TENSORFLOW_CORE_COMMON_RUNTIME_TEST_COLLECTIVE_EXECUTOR_MGR_H_

#include "tensorflow/core/framework/collective.h"
#include "tensorflow/core/framework/device_attributes.pb.h"
#include "tensorflow/core/lib/gtl/flatmap.h"

namespace tensorflow {

// Mock objects that can't actually execute a Collective, but satisfy
// general infrastructure expectations within tests that don't require
// full functionality.

class TestCollectiveExecutor : public CollectiveExecutor {
 public:
  explicit TestCollectiveExecutor(CollectiveExecutorMgrInterface* cem)
      : CollectiveExecutor(cem) {}

  void RunClosure(std::function<void()>) override {
    LOG(FATAL) << "Unimplemented";
  }
};

class TestParamResolver : public ParamResolverInterface {
  void CompleteParamsAsync(const DeviceAttributes& device, CollectiveParams* cp,
                           CancellationManager* cancel_mgr,
                           const StatusCallback& done) override {
    done(errors::Internal("Unimplemented"));
  }

  void CompleteGroupAsync(const CompleteGroupRequest* request,
                          CompleteGroupResponse* response,
                          CancellationManager* cancel_mgr,
                          const StatusCallback& done) override {
    done(errors::Internal("Unimplemented"));
  }

  void CompleteInstanceAsync(const CompleteInstanceRequest* request,
                             CompleteInstanceResponse* response,
                             CancellationManager* cancel_mgr,
                             const StatusCallback& done) override {
    done(errors::Internal("Unimplemented"));
  }

  void StartAbort(const Status& s) override { return; }
};

class TestCollectiveExecutorMgr : public CollectiveExecutorMgrInterface {
 public:
  TestCollectiveExecutorMgr() {}

  ~TestCollectiveExecutorMgr() override {
    for (auto& iter : table_) {
      iter.second->Unref();
    }
  }

  CollectiveExecutor* FindOrCreate(int64 step_id) override {
    mutex_lock l(mu_);
    CollectiveExecutor* ce = nullptr;
    auto iter = table_.find(step_id);
    if (iter != table_.end()) {
      ce = iter->second;
    } else {
      ce = new TestCollectiveExecutor(this);
      table_[step_id] = ce;
    }
    ce->Ref();
    return ce;
  }

  void Cleanup(int64 step_id) override {
    mutex_lock l(mu_);
    auto iter = table_.find(step_id);
    if (iter != table_.end()) {
      iter->second->Unref();
      table_.erase(iter);
    }
  }

  ParamResolverInterface* GetParamResolver() const override {
    return &param_resolver_;
  }

  DeviceResolverInterface* GetDeviceResolver() const override {
    LOG(FATAL);
    return nullptr;
  }

  NcclCommunicatorInterface* GetNcclCommunicator() const override {
    LOG(FATAL) << "Unimplemented";  // Crash OK
    return nullptr;
  }

  void GetStepSequenceAsync(const GetStepSequenceRequest* request,
                            GetStepSequenceResponse* response,
                            const StatusCallback& done) override {
    done(errors::Internal("unimplemented"));
  }

  void RefreshStepIdSequenceAsync(int64 graph_key,
                                  const StatusCallback& done) override {
    done(errors::Internal("unimplemented"));
  }

  int64 NextStepId(int64 graph_key) override {
    return CollectiveExecutor::kInvalidId;
  }

  void RetireStepId(int64 graph_key, int64 step_id) override {}

  mutex mu_;
  gtl::FlatMap<int64, CollectiveExecutor*> table_ TF_GUARDED_BY(mu_);
  mutable TestParamResolver param_resolver_;
};

}  // namespace tensorflow
#endif  // TENSORFLOW_CORE_COMMON_RUNTIME_TEST_COLLECTIVE_EXECUTOR_MGR_H_
