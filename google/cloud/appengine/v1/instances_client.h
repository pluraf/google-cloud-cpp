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

#ifndef GOOGLE_CLOUD_CPP_GOOGLE_CLOUD_APPENGINE_V1_INSTANCES_CLIENT_H
#define GOOGLE_CLOUD_CPP_GOOGLE_CLOUD_APPENGINE_V1_INSTANCES_CLIENT_H

#include "google/cloud/appengine/v1/instances_connection.h"
#include "google/cloud/future.h"
#include "google/cloud/options.h"
#include "google/cloud/polling_policy.h"
#include "google/cloud/status_or.h"
#include "google/cloud/version.h"
#include <google/longrunning/operations.grpc.pb.h>
#include <memory>

namespace google {
namespace cloud {
namespace appengine_v1 {
GOOGLE_CLOUD_CPP_INLINE_NAMESPACE_BEGIN

///
/// Manages instances of a version.
///
/// @par Equality
///
/// Instances of this class created via copy-construction or copy-assignment
/// always compare equal. Instances created with equal
/// `std::shared_ptr<*Connection>` objects compare equal. Objects that compare
/// equal share the same underlying resources.
///
/// @par Performance
///
/// Creating a new instance of this class is a relatively expensive operation,
/// new objects establish new connections to the service. In contrast,
/// copy-construction, move-construction, and the corresponding assignment
/// operations are relatively efficient as the copies share all underlying
/// resources.
///
/// @par Thread Safety
///
/// Concurrent access to different instances of this class, even if they compare
/// equal, is guaranteed to work. Two or more threads operating on the same
/// instance of this class is not guaranteed to work. Since copy-construction
/// and move-construction is a relatively efficient operation, consider using
/// such a copy when using this class from multiple threads.
///
class InstancesClient {
 public:
  explicit InstancesClient(std::shared_ptr<InstancesConnection> connection,
                           Options opts = {});
  ~InstancesClient();

  ///@{
  /// @name Copy and move support
  InstancesClient(InstancesClient const&) = default;
  InstancesClient& operator=(InstancesClient const&) = default;
  InstancesClient(InstancesClient&&) = default;
  InstancesClient& operator=(InstancesClient&&) = default;
  ///@}

  ///@{
  /// @name Equality
  friend bool operator==(InstancesClient const& a, InstancesClient const& b) {
    return a.connection_ == b.connection_;
  }
  friend bool operator!=(InstancesClient const& a, InstancesClient const& b) {
    return !(a == b);
  }
  ///@}

  ///
  /// Lists the instances of a version.
  ///
  /// Tip: To aggregate details about instances over time, see the
  /// [Stackdriver Monitoring
  /// API](https://cloud.google.com/monitoring/api/ref_v3/rest/v3/projects.timeSeries/list).
  ///
  /// @param request
  /// @googleapis_link{google::appengine::v1::ListInstancesRequest,google/appengine/v1/appengine.proto#L492}
  /// @param opts Optional. Override the class-level options, such as retry and
  ///     backoff policies.
  /// @return
  /// @googleapis_link{google::appengine::v1::Instance,google/appengine/v1/instance.proto#L33}
  ///
  /// [google.appengine.v1.Instance]:
  /// @googleapis_reference_link{google/appengine/v1/instance.proto#L33}
  /// [google.appengine.v1.ListInstancesRequest]:
  /// @googleapis_reference_link{google/appengine/v1/appengine.proto#L492}
  ///
  StreamRange<google::appengine::v1::Instance> ListInstances(
      google::appengine::v1::ListInstancesRequest request, Options opts = {});

  ///
  /// Gets instance information.
  ///
  /// @param request
  /// @googleapis_link{google::appengine::v1::GetInstanceRequest,google/appengine/v1/appengine.proto#L514}
  /// @param opts Optional. Override the class-level options, such as retry and
  ///     backoff policies.
  /// @return
  /// @googleapis_link{google::appengine::v1::Instance,google/appengine/v1/instance.proto#L33}
  ///
  /// [google.appengine.v1.GetInstanceRequest]:
  /// @googleapis_reference_link{google/appengine/v1/appengine.proto#L514}
  /// [google.appengine.v1.Instance]:
  /// @googleapis_reference_link{google/appengine/v1/instance.proto#L33}
  ///
  StatusOr<google::appengine::v1::Instance> GetInstance(
      google::appengine::v1::GetInstanceRequest const& request,
      Options opts = {});

  ///
  /// Stops a running instance.
  ///
  /// The instance might be automatically recreated based on the scaling
  /// settings of the version. For more information, see "How Instances are
  /// Managed"
  /// ([standard
  /// environment](https://cloud.google.com/appengine/docs/standard/python/how-instances-are-managed)
  /// | [flexible
  /// environment](https://cloud.google.com/appengine/docs/flexible/python/how-instances-are-managed)).
  ///
  /// To ensure that instances are not re-created and avoid getting billed, you
  /// can stop all instances within the target version by changing the serving
  /// status of the version to `STOPPED` with the
  /// [`apps.services.versions.patch`](https://cloud.google.com/appengine/docs/admin-api/reference/rest/v1/apps.services.versions/patch)
  /// method.
  ///
  /// @param request
  /// @googleapis_link{google::appengine::v1::DeleteInstanceRequest,google/appengine/v1/appengine.proto#L521}
  /// @param opts Optional. Override the class-level options, such as retry and
  ///     backoff policies.
  /// @return
  /// @googleapis_link{google::appengine::v1::OperationMetadataV1,google/appengine/v1/operation.proto#L30}
  ///
  /// [google.appengine.v1.DeleteInstanceRequest]:
  /// @googleapis_reference_link{google/appengine/v1/appengine.proto#L521}
  /// [google.appengine.v1.OperationMetadataV1]:
  /// @googleapis_reference_link{google/appengine/v1/operation.proto#L30}
  ///
  future<StatusOr<google::appengine::v1::OperationMetadataV1>> DeleteInstance(
      google::appengine::v1::DeleteInstanceRequest const& request,
      Options opts = {});

  ///
  /// Enables debugging on a VM instance. This allows you to use the SSH
  /// command to connect to the virtual machine where the instance lives.
  /// While in "debug mode", the instance continues to serve live traffic.
  /// You should delete the instance when you are done debugging and then
  /// allow the system to take over and determine if another instance
  /// should be started.
  ///
  /// Only applicable for instances in App Engine flexible environment.
  ///
  /// @param request
  /// @googleapis_link{google::appengine::v1::DebugInstanceRequest,google/appengine/v1/appengine.proto#L528}
  /// @param opts Optional. Override the class-level options, such as retry and
  ///     backoff policies.
  /// @return
  /// @googleapis_link{google::appengine::v1::Instance,google/appengine/v1/instance.proto#L33}
  ///
  /// [google.appengine.v1.DebugInstanceRequest]:
  /// @googleapis_reference_link{google/appengine/v1/appengine.proto#L528}
  /// [google.appengine.v1.Instance]:
  /// @googleapis_reference_link{google/appengine/v1/instance.proto#L33}
  ///
  future<StatusOr<google::appengine::v1::Instance>> DebugInstance(
      google::appengine::v1::DebugInstanceRequest const& request,
      Options opts = {});

 private:
  std::shared_ptr<InstancesConnection> connection_;
  Options options_;
};

GOOGLE_CLOUD_CPP_INLINE_NAMESPACE_END
}  // namespace appengine_v1
}  // namespace cloud
}  // namespace google

#endif  // GOOGLE_CLOUD_CPP_GOOGLE_CLOUD_APPENGINE_V1_INSTANCES_CLIENT_H
