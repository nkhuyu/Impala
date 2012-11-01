// Copyright 2012 Cloudera Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

//
// A standalone test utility that starts multiple Impala backends and a state store
// within a single process.

#include "codegen/llvm-codegen.h"
#include "common/logging.h"
#include "service/impala-server.h"
#include "testutil/test-exec-env.h"
#include "util/cpu-info.h"
#include "util/disk-info.h"
#include "util/jni-util.h"
#include "util/logging.h"
#include "util/thrift-util.h"
#include "util/thrift-server.h"

DEFINE_int32(num_backends, 3, "The number of backends to start");
DECLARE_int32(be_port);
DECLARE_int32(fe_port);

using namespace impala;
using namespace std;

namespace impala {
TestExecEnv* test_env_;
ThriftServer* fe_server_;
ThriftServer* be_server_;
}

int main(int argc, char** argv) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  InitGoogleLoggingSafe(argv[0]);

  if (FLAGS_num_backends <= 0) {
    LOG(ERROR) << "-num_backends arg must be > 0";
    exit(1);
  }

  InitThriftLogging();
  CpuInfo::Init();
  DiskInfo::Init();
  LlvmCodeGen::InitializeLlvm();
  EXIT_IF_ERROR(JniUtil::Init());

  // Create an in-process Impala server and in-process backends for test environment.
  LOG(INFO) << "Creating test env with " << FLAGS_num_backends << " backends";
  test_env_ = new TestExecEnv(FLAGS_num_backends, FLAGS_be_port + 1);
  LOG(INFO) << "Starting backends";
  test_env_->StartBackends();

  CreateImpalaServer(test_env_, FLAGS_fe_port, FLAGS_be_port, &fe_server_, &be_server_);
  be_server_->Start();
  fe_server_->Start();
  fe_server_->Join();
  delete fe_server_;
  delete be_server_;
}