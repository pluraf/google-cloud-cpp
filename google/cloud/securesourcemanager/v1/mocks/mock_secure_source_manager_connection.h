// Copyright 2023 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Generated by the Codegen C++ plugin.
// If you make any local changes, they will be lost.
// source: google/cloud/securesourcemanager/v1/secure_source_manager.proto

#ifndef GOOGLE_CLOUD_CPP_GOOGLE_CLOUD_SECURESOURCEMANAGER_V1_MOCKS_MOCK_SECURE_SOURCE_MANAGER_CONNECTION_H
#define GOOGLE_CLOUD_CPP_GOOGLE_CLOUD_SECURESOURCEMANAGER_V1_MOCKS_MOCK_SECURE_SOURCE_MANAGER_CONNECTION_H

#include "google/cloud/securesourcemanager/v1/secure_source_manager_connection.h"
#include <gmock/gmock.h>

namespace google {
namespace cloud {
namespace securesourcemanager_v1_mocks {
GOOGLE_CLOUD_CPP_INLINE_NAMESPACE_BEGIN

/**
 * A class to mock `SecureSourceManagerConnection`.
 *
 * Application developers may want to test their code with simulated responses,
 * including errors, from an object of type `SecureSourceManagerClient`. To do
 * so, construct an object of type `SecureSourceManagerClient` with an instance
 * of this class. Then use the Google Test framework functions to program the
 * behavior of this mock.
 *
 * @see [This example][bq-mock] for how to test your application with GoogleTest.
 * While the example showcases types from the BigQuery library, the underlying
 * principles apply for any pair of `*Client` and `*Connection`.
 *
 * [bq-mock]: @cloud_cpp_docs_link{bigquery,bigquery-read-mock}
 */
class MockSecureSourceManagerConnection
    : public securesourcemanager_v1::SecureSourceManagerConnection {
 public:
  MOCK_METHOD(Options, options, (), (override));

  MOCK_METHOD(
      (StreamRange<google::cloud::securesourcemanager::v1::Instance>),
      ListInstances,
      (google::cloud::securesourcemanager::v1::ListInstancesRequest request),
      (override));

  MOCK_METHOD(StatusOr<google::cloud::securesourcemanager::v1::Instance>,
              GetInstance,
              (google::cloud::securesourcemanager::v1::GetInstanceRequest const&
                   request),
              (override));

  MOCK_METHOD(
      future<StatusOr<google::cloud::securesourcemanager::v1::Instance>>,
      CreateInstance,
      (google::cloud::securesourcemanager::v1::CreateInstanceRequest const&
           request),
      (override));

  MOCK_METHOD(
      future<
          StatusOr<google::cloud::securesourcemanager::v1::OperationMetadata>>,
      DeleteInstance,
      (google::cloud::securesourcemanager::v1::DeleteInstanceRequest const&
           request),
      (override));

  MOCK_METHOD(
      (StreamRange<google::cloud::securesourcemanager::v1::Repository>),
      ListRepositories,
      (google::cloud::securesourcemanager::v1::ListRepositoriesRequest request),
      (override));

  MOCK_METHOD(
      StatusOr<google::cloud::securesourcemanager::v1::Repository>,
      GetRepository,
      (google::cloud::securesourcemanager::v1::GetRepositoryRequest const&
           request),
      (override));

  MOCK_METHOD(
      future<StatusOr<google::cloud::securesourcemanager::v1::Repository>>,
      CreateRepository,
      (google::cloud::securesourcemanager::v1::CreateRepositoryRequest const&
           request),
      (override));

  MOCK_METHOD(
      future<
          StatusOr<google::cloud::securesourcemanager::v1::OperationMetadata>>,
      DeleteRepository,
      (google::cloud::securesourcemanager::v1::DeleteRepositoryRequest const&
           request),
      (override));

  MOCK_METHOD(StatusOr<google::iam::v1::Policy>, GetIamPolicyRepo,
              (google::iam::v1::GetIamPolicyRequest const& request),
              (override));

  MOCK_METHOD(StatusOr<google::iam::v1::Policy>, SetIamPolicyRepo,
              (google::iam::v1::SetIamPolicyRequest const& request),
              (override));

  MOCK_METHOD(StatusOr<google::iam::v1::TestIamPermissionsResponse>,
              TestIamPermissionsRepo,
              (google::iam::v1::TestIamPermissionsRequest const& request),
              (override));
};

GOOGLE_CLOUD_CPP_INLINE_NAMESPACE_END
}  // namespace securesourcemanager_v1_mocks
}  // namespace cloud
}  // namespace google

#endif  // GOOGLE_CLOUD_CPP_GOOGLE_CLOUD_SECURESOURCEMANAGER_V1_MOCKS_MOCK_SECURE_SOURCE_MANAGER_CONNECTION_H
