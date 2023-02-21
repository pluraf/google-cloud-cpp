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

#ifndef GOOGLE_CLOUD_CPP_GOOGLE_CLOUD_APPENGINE_V1_FIREWALL_CONNECTION_IDEMPOTENCY_POLICY_H
#define GOOGLE_CLOUD_CPP_GOOGLE_CLOUD_APPENGINE_V1_FIREWALL_CONNECTION_IDEMPOTENCY_POLICY_H

#include "google/cloud/idempotency.h"
#include "google/cloud/internal/retry_policy.h"
#include "google/cloud/version.h"
#include <google/appengine/v1/appengine.grpc.pb.h>
#include <memory>

namespace google {
namespace cloud {
namespace appengine_v1 {
GOOGLE_CLOUD_CPP_INLINE_NAMESPACE_BEGIN

class FirewallConnectionIdempotencyPolicy {
 public:
  virtual ~FirewallConnectionIdempotencyPolicy();

  /// Create a new copy of this object.
  virtual std::unique_ptr<FirewallConnectionIdempotencyPolicy> clone() const;

  virtual google::cloud::Idempotency ListIngressRules(
      google::appengine::v1::ListIngressRulesRequest request);

  virtual google::cloud::Idempotency BatchUpdateIngressRules(
      google::appengine::v1::BatchUpdateIngressRulesRequest const& request);

  virtual google::cloud::Idempotency CreateIngressRule(
      google::appengine::v1::CreateIngressRuleRequest const& request);

  virtual google::cloud::Idempotency GetIngressRule(
      google::appengine::v1::GetIngressRuleRequest const& request);

  virtual google::cloud::Idempotency UpdateIngressRule(
      google::appengine::v1::UpdateIngressRuleRequest const& request);

  virtual google::cloud::Idempotency DeleteIngressRule(
      google::appengine::v1::DeleteIngressRuleRequest const& request);
};

std::unique_ptr<FirewallConnectionIdempotencyPolicy>
MakeDefaultFirewallConnectionIdempotencyPolicy();

GOOGLE_CLOUD_CPP_INLINE_NAMESPACE_END
}  // namespace appengine_v1
}  // namespace cloud
}  // namespace google

#endif  // GOOGLE_CLOUD_CPP_GOOGLE_CLOUD_APPENGINE_V1_FIREWALL_CONNECTION_IDEMPOTENCY_POLICY_H
