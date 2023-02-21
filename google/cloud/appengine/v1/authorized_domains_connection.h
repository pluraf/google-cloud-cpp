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

#ifndef GOOGLE_CLOUD_CPP_GOOGLE_CLOUD_APPENGINE_V1_AUTHORIZED_DOMAINS_CONNECTION_H
#define GOOGLE_CLOUD_CPP_GOOGLE_CLOUD_APPENGINE_V1_AUTHORIZED_DOMAINS_CONNECTION_H

#include "google/cloud/appengine/v1/authorized_domains_connection_idempotency_policy.h"
#include "google/cloud/appengine/v1/internal/authorized_domains_retry_traits.h"
#include "google/cloud/appengine/v1/internal/authorized_domains_stub.h"
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

using AuthorizedDomainsRetryPolicy =
    ::google::cloud::internal::TraitBasedRetryPolicy<
        appengine_v1_internal::AuthorizedDomainsRetryTraits>;

using AuthorizedDomainsLimitedTimeRetryPolicy =
    ::google::cloud::internal::LimitedTimeRetryPolicy<
        appengine_v1_internal::AuthorizedDomainsRetryTraits>;

using AuthorizedDomainsLimitedErrorCountRetryPolicy =
    ::google::cloud::internal::LimitedErrorCountRetryPolicy<
        appengine_v1_internal::AuthorizedDomainsRetryTraits>;

/**
 * The `AuthorizedDomainsConnection` object for `AuthorizedDomainsClient`.
 *
 * This interface defines virtual methods for each of the user-facing overload
 * sets in `AuthorizedDomainsClient`. This allows users to inject custom
 * behavior (e.g., with a Google Mock object) when writing tests that use
 * objects of type `AuthorizedDomainsClient`.
 *
 * To create a concrete instance, see `MakeAuthorizedDomainsConnection()`.
 *
 * For mocking, see `appengine_v1_mocks::MockAuthorizedDomainsConnection`.
 */
class AuthorizedDomainsConnection {
 public:
  virtual ~AuthorizedDomainsConnection() = 0;

  virtual Options options() { return Options{}; }

  virtual StreamRange<google::appengine::v1::AuthorizedDomain>
  ListAuthorizedDomains(
      google::appengine::v1::ListAuthorizedDomainsRequest request);
};

/**
 * A factory function to construct an object of type
 * `AuthorizedDomainsConnection`.
 *
 * The returned connection object should not be used directly; instead it
 * should be passed as an argument to the constructor of
 * AuthorizedDomainsClient.
 *
 * The optional @p options argument may be used to configure aspects of the
 * returned `AuthorizedDomainsConnection`. Expected options are any of the types
 * in the following option lists:
 *
 * - `google::cloud::CommonOptionList`
 * - `google::cloud::GrpcOptionList`
 * - `google::cloud::UnifiedCredentialsOptionList`
 * - `google::cloud::appengine_v1::AuthorizedDomainsPolicyOptionList`
 *
 * @note Unexpected options will be ignored. To log unexpected options instead,
 *     set `GOOGLE_CLOUD_CPP_ENABLE_CLOG=yes` in the environment.
 *
 * @param options (optional) Configure the `AuthorizedDomainsConnection` created
 * by this function.
 */
std::shared_ptr<AuthorizedDomainsConnection> MakeAuthorizedDomainsConnection(
    Options options = {});

GOOGLE_CLOUD_CPP_INLINE_NAMESPACE_END
}  // namespace appengine_v1
}  // namespace cloud
}  // namespace google

#endif  // GOOGLE_CLOUD_CPP_GOOGLE_CLOUD_APPENGINE_V1_AUTHORIZED_DOMAINS_CONNECTION_H
