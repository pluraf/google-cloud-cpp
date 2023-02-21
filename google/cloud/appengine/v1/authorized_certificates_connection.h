// Copyright 2022 Google LLC
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
// source: google/appengine/v1/appengine.proto

#ifndef GOOGLE_CLOUD_CPP_GOOGLE_CLOUD_APPENGINE_V1_AUTHORIZED_CERTIFICATES_CONNECTION_H
#define GOOGLE_CLOUD_CPP_GOOGLE_CLOUD_APPENGINE_V1_AUTHORIZED_CERTIFICATES_CONNECTION_H

#include "google/cloud/appengine/v1/authorized_certificates_connection_idempotency_policy.h"
#include "google/cloud/appengine/v1/internal/authorized_certificates_retry_traits.h"
#include "google/cloud/appengine/v1/internal/authorized_certificates_stub.h"
#include "google/cloud/backoff_policy.h"
#include "google/cloud/options.h"
#include "google/cloud/status_or.h"
#include "google/cloud/stream_range.h"
#include "google/cloud/version.h"
#include <memory>

namespace google {
namespace cloud {
namespace appengine_v1 {
GOOGLE_CLOUD_CPP_INLINE_NAMESPACE_BEGIN

using AuthorizedCertificatesRetryPolicy =
    ::google::cloud::internal::TraitBasedRetryPolicy<
        appengine_v1_internal::AuthorizedCertificatesRetryTraits>;

using AuthorizedCertificatesLimitedTimeRetryPolicy =
    ::google::cloud::internal::LimitedTimeRetryPolicy<
        appengine_v1_internal::AuthorizedCertificatesRetryTraits>;

using AuthorizedCertificatesLimitedErrorCountRetryPolicy =
    ::google::cloud::internal::LimitedErrorCountRetryPolicy<
        appengine_v1_internal::AuthorizedCertificatesRetryTraits>;

/**
 * The `AuthorizedCertificatesConnection` object for
 * `AuthorizedCertificatesClient`.
 *
 * This interface defines virtual methods for each of the user-facing overload
 * sets in `AuthorizedCertificatesClient`. This allows users to inject custom
 * behavior (e.g., with a Google Mock object) when writing tests that use
 * objects of type `AuthorizedCertificatesClient`.
 *
 * To create a concrete instance, see `MakeAuthorizedCertificatesConnection()`.
 *
 * For mocking, see `appengine_v1_mocks::MockAuthorizedCertificatesConnection`.
 */
class AuthorizedCertificatesConnection {
 public:
  virtual ~AuthorizedCertificatesConnection() = 0;

  virtual Options options() { return Options{}; }

  virtual StreamRange<google::appengine::v1::AuthorizedCertificate>
  ListAuthorizedCertificates(
      google::appengine::v1::ListAuthorizedCertificatesRequest request);

  virtual StatusOr<google::appengine::v1::AuthorizedCertificate>
  GetAuthorizedCertificate(
      google::appengine::v1::GetAuthorizedCertificateRequest const& request);

  virtual StatusOr<google::appengine::v1::AuthorizedCertificate>
  CreateAuthorizedCertificate(
      google::appengine::v1::CreateAuthorizedCertificateRequest const& request);

  virtual StatusOr<google::appengine::v1::AuthorizedCertificate>
  UpdateAuthorizedCertificate(
      google::appengine::v1::UpdateAuthorizedCertificateRequest const& request);

  virtual Status DeleteAuthorizedCertificate(
      google::appengine::v1::DeleteAuthorizedCertificateRequest const& request);
};

/**
 * A factory function to construct an object of type
 * `AuthorizedCertificatesConnection`.
 *
 * The returned connection object should not be used directly; instead it
 * should be passed as an argument to the constructor of
 * AuthorizedCertificatesClient.
 *
 * The optional @p options argument may be used to configure aspects of the
 * returned `AuthorizedCertificatesConnection`. Expected options are any of the
 * types in the following option lists:
 *
 * - `google::cloud::CommonOptionList`
 * - `google::cloud::GrpcOptionList`
 * - `google::cloud::UnifiedCredentialsOptionList`
 * - `google::cloud::appengine_v1::AuthorizedCertificatesPolicyOptionList`
 *
 * @note Unexpected options will be ignored. To log unexpected options instead,
 *     set `GOOGLE_CLOUD_CPP_ENABLE_CLOG=yes` in the environment.
 *
 * @param options (optional) Configure the `AuthorizedCertificatesConnection`
 * created by this function.
 */
std::shared_ptr<AuthorizedCertificatesConnection>
MakeAuthorizedCertificatesConnection(Options options = {});

GOOGLE_CLOUD_CPP_INLINE_NAMESPACE_END
}  // namespace appengine_v1
}  // namespace cloud
}  // namespace google

#endif  // GOOGLE_CLOUD_CPP_GOOGLE_CLOUD_APPENGINE_V1_AUTHORIZED_CERTIFICATES_CONNECTION_H
