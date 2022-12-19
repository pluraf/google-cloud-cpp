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

#include "generator/internal/forwarding_connection_generator.h"

namespace google {
namespace cloud {
namespace generator_internal {

ForwardingConnectionGenerator::ForwardingConnectionGenerator(
    google::protobuf::ServiceDescriptor const* service_descriptor,
    VarsDictionary service_vars,
    std::map<std::string, VarsDictionary> service_method_vars,
    google::protobuf::compiler::GeneratorContext* context)
    : ServiceCodeGenerator("forwarding_connection_header_path",
                           service_descriptor, std::move(service_vars),
                           std::move(service_method_vars), context) {}

Status ForwardingConnectionGenerator::GenerateHeader() {
  HeaderPrint(CopyrightLicenseFileHeader());
  HeaderPrint(R"""(
// Generated by the Codegen C++ plugin.
// If you make any local changes, they will be lost.
// source: $proto_file_name$

#ifndef $header_include_guard$
#define $header_include_guard$

)""");

  // includes
  HeaderLocalIncludes({
      vars("forwarding_idempotency_policy_header_path"),
      vars("connection_header_path"),
  });

  auto result = HeaderOpenForwardingNamespaces();
  if (!result.ok()) return result;

  HeaderPrint(
      R"""(
using ::google::cloud::$product_namespace$::Make$connection_class_name$;
using ::google::cloud::$product_namespace$::$connection_class_name$;
using ::google::cloud::$product_namespace$::$limited_error_count_retry_policy_name$;
using ::google::cloud::$product_namespace$::$limited_time_retry_policy_name$;
using ::google::cloud::$product_namespace$::$retry_policy_name$;
)""");

  HeaderCloseNamespaces();
  // close header guard
  HeaderPrint(R"""(
#endif  // $header_include_guard$
)""");
  return {};
}

}  // namespace generator_internal
}  // namespace cloud
}  // namespace google
