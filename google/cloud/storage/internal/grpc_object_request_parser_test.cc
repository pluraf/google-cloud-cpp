// Copyright 2020 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "google/cloud/storage/internal/grpc_object_request_parser.h"
#include "google/cloud/storage/internal/grpc_client.h"
#include "google/cloud/storage/oauth2/google_credentials.h"
#include "google/cloud/grpc_options.h"
#include "google/cloud/testing_util/is_proto_equal.h"
#include "google/cloud/testing_util/scoped_environment.h"
#include "google/cloud/testing_util/status_matchers.h"
#include <google/protobuf/text_format.h>
#include <gmock/gmock.h>

namespace google {
namespace cloud {
namespace storage {
GOOGLE_CLOUD_CPP_INLINE_NAMESPACE_BEGIN
namespace internal {
namespace {

namespace storage_proto = ::google::storage::v2;
using ::google::cloud::testing_util::IsProtoEqual;
using ::google::cloud::testing_util::StatusIs;
using ::google::protobuf::TextFormat;
using ::testing::ElementsAre;
using ::testing::Pair;
using ::testing::UnorderedElementsAre;

// Use gsutil to obtain the CRC32C checksum (in base64):
//    TEXT="The quick brown fox jumps over the lazy dog"
//    /bin/echo -n $TEXT > /tmp/fox.txt
//    gsutil hash /tmp/fox.txt
// Hashes [base64] for /tmp/fox.txt:
//    Hash (crc32c): ImIEBA==
//    Hash (md5)   : nhB9nTcrtoJr2B01QqQZ1g==
//
// Then convert the base64 values to hex
//
//     echo "ImIEBA==" | openssl base64 -d | od -t x1
//     echo "nhB9nTcrtoJr2B01QqQZ1g==" | openssl base64 -d | od -t x1
//
// Which yields (in proto format):
//
//     CRC32C      : 0x22620404
//     MD5         : 9e107d9d372bb6826bd81d3542a419d6
auto constexpr kText = "The quick brown fox jumps over the lazy dog";

// Doing something similar for an alternative text yields:
// Hashes [base64] for /tmp/alt.txt:
//    Hash (crc32c): StZ/gA==
//    Hash (md5)   : StEvo2V/qoDCuaktZSw3IQ==
// In proto format
//     CRC32C      : 0x4ad67f80
//     MD5         : 4ad12fa3657faa80c2b9a92d652c3721
auto constexpr kAlt = "How vexingly quick daft zebras jump!";

// Many of the tests need to verify that all fields can be set when creating
// or updating objects. The next two functions provide most of the values for
// such objects. There are a few edge conditions:
// - Some fields, like `storage_class`, an only be set in create operations,
//   we leave those undefined here, and explicitly set them in each test
// - Some fields, like the object name and bucket, are required in some gRPC
//   requests, but not others. We also leave those undefined here.
// - Some fields, like `kms_key`, can be set via an option or via the object
//   metadata. We leave those undefined here too.
google::storage::v2::Object ExpectedFullObjectMetadata() {
  // The fields are sorted as they appear in the .proto file.
  auto constexpr kProto = R"pb(
    # storage_class: "REGIONAL" ## set only where applicable
    content_encoding: "test-content-encoding"
    content_disposition: "test-content-disposition"
    cache_control: "test-cache-control"
    acl: { role: "test-role1" entity: "test-entity1" }
    acl: { role: "test-role2" entity: "test-entity2" }
    content_language: "test-content-language"
    content_type: "test-content-type"
    temporary_hold: true
    metadata: { key: "test-metadata-key1" value: "test-value1" }
    metadata: { key: "test-metadata-key2" value: "test-value2" }
    event_based_hold: true
    custom_time { seconds: 1643126687 nanos: 123000000 }
  )pb";
  google::storage::v2::Object proto;
  if (TextFormat::ParseFromString(kProto, &proto)) return proto;
  ADD_FAILURE() << "Parsing text proto for " << __func__ << " failed";
  return proto;
}

ObjectMetadata FullObjectMetadata() {
  return ObjectMetadata{}
      .set_content_encoding("test-content-encoding")
      .set_content_disposition("test-content-disposition")
      .set_cache_control("test-cache-control")
      .set_acl({ObjectAccessControl()
                    .set_role("test-role1")
                    .set_entity("test-entity1"),
                ObjectAccessControl()
                    .set_role("test-role2")
                    .set_entity("test-entity2")})
      .set_content_language("test-content-language")
      .set_content_type("test-content-type")
      .set_temporary_hold(true)
      .upsert_metadata("test-metadata-key1", "test-value1")
      .upsert_metadata("test-metadata-key2", "test-value2")
      .set_event_based_hold(true)
      .set_custom_time(std::chrono::system_clock::time_point{} +
                       std::chrono::seconds(1643126687) +
                       std::chrono::milliseconds(123));
}

google::storage::v2::CommonObjectRequestParams
ExpectedCommonObjectRequestParams() {
  // To get the magic values use:
  //  /bin/echo -n "01234567" | sha256sum
  auto constexpr kProto = R"pb(
    encryption_algorithm: "AES256"
    encryption_key_bytes: "01234567"
    encryption_key_sha256_bytes: "\x92\x45\x92\xb9\xb1\x03\xf1\x4f\x83\x3f\xaa\xfb\x67\xf4\x80\x69\x1f\x01\x98\x8a\xa4\x57\xc0\x06\x17\x69\xf5\x8c\xd4\x73\x11\xbc"
  )pb";
  google::storage::v2::CommonObjectRequestParams proto;
  if (TextFormat::ParseFromString(kProto, &proto)) return proto;
  ADD_FAILURE() << "Parsing text proto for " << __func__ << " failed";
  return proto;
}

TEST(GrpcObjectRequestParser, ComposeObjectRequestAllOptions) {
  auto constexpr kTextProto = R"pb(
    source_objects { name: "source-object-1" }
    source_objects {
      name: "source-object-2"
      generation: 27
      object_preconditions { if_generation_match: 28 }
    }
    source_objects { name: "source-object-3" generation: 37 }
    source_objects {
      name: "source-object-4"
      object_preconditions { if_generation_match: 48 }
    }
    destination_predefined_acl: "projectPrivate"
    if_generation_match: 1
    if_metageneration_match: 3
    kms_key: "test-only-kms-key"
  )pb";
  google::storage::v2::ComposeObjectRequest expected;
  ASSERT_TRUE(TextFormat::ParseFromString(kTextProto, &expected));
  auto& destination = *expected.mutable_destination();
  destination = ExpectedFullObjectMetadata();
  destination.set_bucket("projects/_/buckets/bucket-name");
  destination.set_name("object-name");
  destination.set_storage_class("STANDARD");
  *expected.mutable_common_object_request_params() =
      ExpectedCommonObjectRequestParams();

  ComposeObjectRequest req(
      "bucket-name",
      {
          ComposeSourceObject{"source-object-1", absl::nullopt, absl::nullopt},
          ComposeSourceObject{"source-object-2", 27, 28},
          ComposeSourceObject{"source-object-3", 37, absl::nullopt},
          ComposeSourceObject{"source-object-4", absl::nullopt, 48},
      },
      "object-name");
  req.set_multiple_options(
      EncryptionKey::FromBinaryKey("01234567"),
      DestinationPredefinedAcl("projectPrivate"),
      KmsKeyName("test-only-kms-key"), IfGenerationMatch(1),
      IfMetagenerationMatch(3), UserProject("test-user-project"),
      WithObjectMetadata(FullObjectMetadata().set_storage_class("STANDARD")),
      QuotaUser("test-quota-user"), UserIp("test-user-ip"));

  auto actual = GrpcObjectRequestParser::ToProto(req);
  ASSERT_STATUS_OK(actual);
  EXPECT_THAT(*actual, IsProtoEqual(expected));
}

TEST(GrpcObjectRequestParser, DeleteObjectAllFields) {
  google::storage::v2::DeleteObjectRequest expected;
  EXPECT_TRUE(TextFormat::ParseFromString(
      R"pb(
        bucket: "projects/_/buckets/test-bucket"
        object: "test-object"
        generation: 7
        if_generation_match: 1
        if_generation_not_match: 2
        if_metageneration_match: 3
        if_metageneration_not_match: 4
      )pb",
      &expected));

  DeleteObjectRequest req("test-bucket", "test-object");
  req.set_multiple_options(
      Generation(7), IfGenerationMatch(1), IfGenerationNotMatch(2),
      IfMetagenerationMatch(3), IfMetagenerationNotMatch(4),
      UserProject("test-user-project"), QuotaUser("test-quota-user"),
      UserIp("test-user-ip"));

  auto const actual = GrpcObjectRequestParser::ToProto(req);
  EXPECT_THAT(actual, IsProtoEqual(expected));
}

TEST(GrpcObjectRequestParser, GetObjectMetadataAllFields) {
  google::storage::v2::GetObjectRequest expected;
  EXPECT_TRUE(TextFormat::ParseFromString(
      R"pb(
        bucket: "projects/_/buckets/test-bucket"
        object: "test-object"
        generation: 7
        if_generation_match: 1
        if_generation_not_match: 2
        if_metageneration_match: 3
        if_metageneration_not_match: 4
        read_mask { paths: "*" }
      )pb",
      &expected));

  GetObjectMetadataRequest req("test-bucket", "test-object");
  req.set_multiple_options(
      Generation(7), IfGenerationMatch(1), IfGenerationNotMatch(2),
      IfMetagenerationMatch(3), IfMetagenerationNotMatch(4), Projection("full"),
      UserProject("test-user-project"), UserProject("test-user-project"),
      QuotaUser("test-quota-user"), UserIp("test-user-ip"));

  auto const actual = GrpcObjectRequestParser::ToProto(req);
  EXPECT_THAT(actual, IsProtoEqual(expected));
}

TEST(GrpcObjectRequestParser, ReadObjectRangeRequestSimple) {
  google::storage::v2::ReadObjectRequest expected;
  EXPECT_TRUE(TextFormat::ParseFromString(
      R"pb(
        bucket: "projects/_/buckets/test-bucket" object: "test-object"
      )pb",
      &expected));

  ReadObjectRangeRequest req("test-bucket", "test-object");

  auto const actual = GrpcObjectRequestParser::ToProto(req).value();
  EXPECT_THAT(actual, IsProtoEqual(expected));
}

TEST(GrpcObjectRequestParser, ReadObjectRangeRequestAllFields) {
  google::storage::v2::ReadObjectRequest expected;
  EXPECT_TRUE(TextFormat::ParseFromString(
      R"pb(
        bucket: "projects/_/buckets/test-bucket"
        object: "test-object"
        generation: 7
        read_offset: 2000
        read_limit: 1000
        if_generation_match: 1
        if_generation_not_match: 2
        if_metageneration_match: 3
        if_metageneration_not_match: 4
      )pb",
      &expected));
  *expected.mutable_common_object_request_params() =
      ExpectedCommonObjectRequestParams();

  ReadObjectRangeRequest req("test-bucket", "test-object");
  req.set_multiple_options(
      Generation(7), ReadFromOffset(2000), ReadRange(1000, 3000),
      IfGenerationMatch(1), IfGenerationNotMatch(2), IfMetagenerationMatch(3),
      IfMetagenerationNotMatch(4), UserProject("test-user-project"),
      UserProject("test-user-project"), QuotaUser("test-quota-user"),
      UserIp("test-user-ip"), EncryptionKey::FromBinaryKey("01234567"));

  auto const actual = GrpcObjectRequestParser::ToProto(req).value();
  EXPECT_THAT(actual, IsProtoEqual(expected));
}

TEST(GrpcObjectRequestParser, ReadObjectRangeRequestReadLast) {
  google::storage::v2::ReadObjectRequest expected;
  EXPECT_TRUE(TextFormat::ParseFromString(
      R"pb(
        bucket: "projects/_/buckets/test-bucket"
        object: "test-object"
        read_offset: -2000
      )pb",
      &expected));

  ReadObjectRangeRequest req("test-bucket", "test-object");
  req.set_multiple_options(ReadLast(2000));

  auto const actual = GrpcObjectRequestParser::ToProto(req).value();
  EXPECT_THAT(actual, IsProtoEqual(expected));
}

TEST(GrpcObjectRequestParser, ReadObjectRangeRequestReadLastZero) {
  google::storage::v2::ReadObjectRequest expected;
  EXPECT_TRUE(TextFormat::ParseFromString(
      R"pb(
        bucket: "projects/_/buckets/test-bucket" object: "test-object"
      )pb",
      &expected));

  ReadObjectRangeRequest req("test-bucket", "test-object");
  req.set_multiple_options(ReadLast(0));

  auto const actual = GrpcObjectRequestParser::ToProto(req).value();
  EXPECT_THAT(actual, IsProtoEqual(expected));

  auto client = GrpcClient::Create(DefaultOptionsGrpc(
      Options{}
          .set<GrpcCredentialOption>(grpc::InsecureChannelCredentials())
          .set<EndpointOption>("localhost:1")));
  StatusOr<std::unique_ptr<ObjectReadSource>> reader = client->ReadObject(req);
  EXPECT_THAT(reader, StatusIs(StatusCode::kOutOfRange));
}

TEST(GrpcObjectRequestParser, PatchObjectRequestAllOptions) {
  auto constexpr kTextProto = R"pb(
    predefined_acl: "projectPrivate"
    if_generation_match: 1
    if_generation_not_match: 2
    if_metageneration_match: 3
    if_metageneration_not_match: 4
    update_mask {}
  )pb";
  google::storage::v2::UpdateObjectRequest expected;
  ASSERT_TRUE(TextFormat::ParseFromString(kTextProto, &expected));
  auto& object = *expected.mutable_object();
  object = ExpectedFullObjectMetadata();
  object.set_name("object-name");
  object.set_bucket("projects/_/buckets/bucket-name");
  object.set_generation(7);
  *expected.mutable_common_object_request_params() =
      ExpectedCommonObjectRequestParams();

  PatchObjectRequest req(
      "bucket-name", "object-name",
      ObjectMetadataPatchBuilder{}
          .SetContentEncoding("test-content-encoding")
          .SetContentDisposition("test-content-disposition")
          .SetCacheControl("test-cache-control")
          .SetContentLanguage("test-content-language")
          .SetContentType("test-content-type")
          .SetMetadata("test-metadata-key1", "test-value1")
          .SetMetadata("test-metadata-key2", "test-value2")
          .SetTemporaryHold(true)
          .SetAcl({
              ObjectAccessControl{}
                  .set_entity("test-entity1")
                  .set_role("test-role1"),
              ObjectAccessControl{}
                  .set_entity("test-entity2")
                  .set_role("test-role2"),
          })
          .SetEventBasedHold(true)
          .SetCustomTime(std::chrono::system_clock::time_point{} +
                         std::chrono::seconds(1643126687) +
                         std::chrono::milliseconds(123)));
  req.set_multiple_options(
      Generation(7), IfGenerationMatch(1), IfGenerationNotMatch(2),
      IfMetagenerationMatch(3), IfMetagenerationNotMatch(4),
      PredefinedAcl("projectPrivate"), EncryptionKey::FromBinaryKey("01234567"),
      Projection("full"), UserProject("test-user-project"),
      QuotaUser("test-quota-user"), UserIp("test-user-ip"));

  auto actual = GrpcObjectRequestParser::ToProto(req);
  ASSERT_STATUS_OK(actual);
  // First check the paths. We do not care about their order, so checking them
  // with IsProtoEqual does not work.
  EXPECT_THAT(
      actual->update_mask().paths(),
      UnorderedElementsAre("acl", "content_encoding", "content_disposition",
                           "cache_control", "content_language", "content_type",
                           "metadata", "temporary_hold", "event_based_hold",
                           "custom_time"));
  // Clear the paths, which we already compared, and compare the proto.
  actual->mutable_update_mask()->clear_paths();
  EXPECT_THAT(*actual, IsProtoEqual(expected));
}

TEST(GrpcObjectRequestParser, PatchObjectRequestAllResets) {
  auto constexpr kTextProto = R"pb(
    object { bucket: "projects/_/buckets/bucket-name" name: "object-name" }
    update_mask {}
  )pb";
  google::storage::v2::UpdateObjectRequest expected;
  ASSERT_TRUE(TextFormat::ParseFromString(kTextProto, &expected));

  PatchObjectRequest req("bucket-name", "object-name",
                         ObjectMetadataPatchBuilder{}
                             .ResetAcl()
                             .ResetCacheControl()
                             .ResetContentDisposition()
                             .ResetContentEncoding()
                             .ResetContentLanguage()
                             .ResetContentType()
                             .ResetEventBasedHold()
                             .ResetMetadata()
                             .ResetTemporaryHold()
                             .ResetCustomTime());

  auto actual = GrpcObjectRequestParser::ToProto(req);
  ASSERT_STATUS_OK(actual);
  // First check the paths. We do not care about their order, so checking them
  // with IsProtoEqual does not work.
  EXPECT_THAT(
      actual->update_mask().paths(),
      UnorderedElementsAre("acl", "content_encoding", "content_disposition",
                           "cache_control", "content_language", "content_type",
                           "metadata", "temporary_hold", "event_based_hold",
                           "custom_time"));
  // Clear the paths, which we already compared, and compare the proto.
  actual->mutable_update_mask()->clear_paths();
  EXPECT_THAT(*actual, IsProtoEqual(expected));
}

TEST(GrpcObjectRequestParser, UpdateObjectRequestAllOptions) {
  auto constexpr kTextProto = R"pb(
    predefined_acl: "projectPrivate"
    if_generation_match: 1
    if_generation_not_match: 2
    if_metageneration_match: 3
    if_metageneration_not_match: 4
    update_mask {}
  )pb";
  google::storage::v2::UpdateObjectRequest expected;
  ASSERT_TRUE(TextFormat::ParseFromString(kTextProto, &expected));
  auto& object = *expected.mutable_object();
  object = ExpectedFullObjectMetadata();
  object.set_bucket("projects/_/buckets/bucket-name");
  object.set_name("object-name");
  object.set_generation(7);
  *expected.mutable_common_object_request_params() =
      ExpectedCommonObjectRequestParams();

  UpdateObjectRequest req("bucket-name", "object-name", FullObjectMetadata());
  req.set_multiple_options(
      Generation(7), IfGenerationMatch(1), IfGenerationNotMatch(2),
      IfMetagenerationMatch(3), IfMetagenerationNotMatch(4),
      PredefinedAcl("projectPrivate"), EncryptionKey::FromBinaryKey("01234567"),
      Projection("full"), UserProject("test-user-project"),
      QuotaUser("test-quota-user"), UserIp("test-user-ip"));

  auto actual = GrpcObjectRequestParser::ToProto(req);
  ASSERT_STATUS_OK(actual);
  // First check the paths, we do not care about their order, so checking them
  // with IsProtoEqual does not work.
  EXPECT_THAT(
      actual->update_mask().paths(),
      UnorderedElementsAre("acl", "content_encoding", "content_disposition",
                           "cache_control", "content_language", "content_type",
                           "metadata", "temporary_hold", "event_based_hold",
                           "custom_time"));
  // Clear the paths, which we already compared, and test the rest
  actual->mutable_update_mask()->clear_paths();
  EXPECT_THAT(*actual, IsProtoEqual(expected));
}

TEST(GrpcObjectRequestParser, InsertObjectMediaRequestSimple) {
  storage_proto::WriteObjectRequest expected;
  EXPECT_TRUE(TextFormat::ParseFromString(
      R"pb(
        write_object_spec: {
          resource: {
            bucket: "projects/_/buckets/test-bucket-name"
            name: "test-object-name"
          }
        }
        object_checksums: {
          # See top-of-file comments for details on the magic numbers
          crc32c: 0x22620404
          # MD5 hashes are disabled by default
          # md5_hash: "9e107d9d372bb6826bd81d3542a419d6"
        }
      )pb",
      &expected));

  InsertObjectMediaRequest request(
      "test-bucket-name", "test-object-name",
      "The quick brown fox jumps over the lazy dog");
  auto actual = GrpcObjectRequestParser::ToProto(request).value();
  EXPECT_THAT(actual, IsProtoEqual(expected));
}

TEST(GrpcObjectRequestParser, InsertObjectMediaRequestHashOptions) {
  // See top-of-file comments for details on the magic numbers
  struct Test {
    std::function<void(InsertObjectMediaRequest&)> apply_options;
    std::string expected_checksums;
  } cases[] = {
      // These tests provide the "wrong" hashes. This is what would happen if
      // one was (for example) reading a GCS file, obtained the expected hashes
      // from GCS, and then uploaded to another GCS destination *but* the data
      // was somehow corrupted locally (say a bad disk). In that case, we don't
      // want to recompute the hashes in the upload.
      {
          [](InsertObjectMediaRequest& r) {
            r.set_option(MD5HashValue(ComputeMD5Hash(kText)));
            r.set_option(DisableCrc32cChecksum(true));
          },
          R"pb(
            md5_hash: "\x9e\x10\x7d\x9d\x37\x2b\xb6\x82\x6b\xd8\x1d\x35\x42\xa4\x19\xd6")pb",
      },
      {
          [](InsertObjectMediaRequest& r) {
            r.set_option(MD5HashValue(ComputeMD5Hash(kText)));
            r.set_option(DisableCrc32cChecksum(false));
          },
          R"pb(
            md5_hash: "\x9e\x10\x7d\x9d\x37\x2b\xb6\x82\x6b\xd8\x1d\x35\x42\xa4\x19\xd6"
            crc32c: 0x4ad67f80)pb",
      },
      {
          [](InsertObjectMediaRequest& r) {
            r.set_option(MD5HashValue(ComputeMD5Hash(kText)));
            r.set_option(Crc32cChecksumValue(ComputeCrc32cChecksum(kText)));
          },
          R"pb(
            md5_hash: "\x9e\x10\x7d\x9d\x37\x2b\xb6\x82\x6b\xd8\x1d\x35\x42\xa4\x19\xd6"
            crc32c: 0x22620404)pb",
      },

      {
          [](InsertObjectMediaRequest& r) {
            r.set_option(DisableMD5Hash(false));
            r.set_option(DisableCrc32cChecksum(true));
          },
          R"pb(
            md5_hash: "\x4a\xd1\x2f\xa3\x65\x7f\xaa\x80\xc2\xb9\xa9\x2d\x65\x2c\x37\x21")pb",
      },
      {
          [](InsertObjectMediaRequest& r) {
            r.set_option(DisableMD5Hash(false));
            r.set_option(DisableCrc32cChecksum(false));
          },
          R"pb(
            md5_hash: "\x4a\xd1\x2f\xa3\x65\x7f\xaa\x80\xc2\xb9\xa9\x2d\x65\x2c\x37\x21"
            crc32c: 0x4ad67f80)pb",
      },
      {
          [](InsertObjectMediaRequest& r) {
            r.set_option(DisableMD5Hash(false));
            r.set_option(Crc32cChecksumValue(ComputeCrc32cChecksum(kText)));
          },
          R"pb(
            md5_hash: "\x4a\xd1\x2f\xa3\x65\x7f\xaa\x80\xc2\xb9\xa9\x2d\x65\x2c\x37\x21"
            crc32c: 0x22620404)pb",
      },

      {
          [](InsertObjectMediaRequest& r) {
            r.set_option(DisableMD5Hash(true));
            r.set_option(DisableCrc32cChecksum(true));
          },
          R"pb(
          )pb",
      },
      {
          [](InsertObjectMediaRequest& r) {
            r.set_option(DisableMD5Hash(true));
            r.set_option(DisableCrc32cChecksum(false));
          },
          R"pb(
            crc32c: 0x4ad67f80)pb",
      },
      {
          [](InsertObjectMediaRequest& r) {
            r.set_option(DisableMD5Hash(true));
            r.set_option(Crc32cChecksumValue(ComputeCrc32cChecksum(kText)));
          },
          R"pb(
            crc32c: 0x22620404)pb",
      },
  };

  for (auto const& test : cases) {
    SCOPED_TRACE("Expected outcome " + test.expected_checksums);
    storage_proto::ObjectChecksums expected;
    ASSERT_TRUE(
        TextFormat::ParseFromString(test.expected_checksums, &expected));

    InsertObjectMediaRequest request("test-bucket-name", "test-object-name",
                                     kAlt);
    test.apply_options(request);
    auto actual = GrpcObjectRequestParser::ToProto(request);
    ASSERT_STATUS_OK(actual) << "expected=" << test.expected_checksums;
    EXPECT_THAT(actual->object_checksums(), IsProtoEqual(expected));
  }
}

TEST(GrpcObjectRequestParser, InsertObjectMediaRequestAllOptions) {
  auto constexpr kTextProto =
      R"pb(
    write_object_spec: {
      resource: {
        bucket: "projects/_/buckets/test-bucket-name"
        name: "test-object-name"
        content_type: "test-content-type"
        content_encoding: "test-content-encoding"
        # Should not be set, the proto file says these values should
        # not be included in the upload
        #     crc32c:
        #     md5_hash:
        kms_key: "test-kms-key-name"
      }
      predefined_acl: "private"
      if_generation_match: 0
      if_generation_not_match: 7
      if_metageneration_match: 42
      if_metageneration_not_match: 84
    }
    object_checksums: {
      # See top-of-file comments for details on the magic numbers
      crc32c: 0x22620404
      md5_hash: "\x9e\x10\x7d\x9d\x37\x2b\xb6\x82\x6b\xd8\x1d\x35\x42\xa4\x19\xd6"
    })pb";
  storage_proto::WriteObjectRequest expected;
  EXPECT_TRUE(TextFormat::ParseFromString(kTextProto, &expected));
  *expected.mutable_common_object_request_params() =
      ExpectedCommonObjectRequestParams();

  auto constexpr kContents = "The quick brown fox jumps over the lazy dog";

  InsertObjectMediaRequest request("test-bucket-name", "test-object-name",
                                   kContents);
  request.set_multiple_options(
      ContentType("test-content-type"),
      ContentEncoding("test-content-encoding"),
      Crc32cChecksumValue(ComputeCrc32cChecksum(kContents)),
      MD5HashValue(ComputeMD5Hash(kContents)), PredefinedAcl("private"),
      IfGenerationMatch(0), IfGenerationNotMatch(7), IfMetagenerationMatch(42),
      IfMetagenerationNotMatch(84), Projection::Full(),
      UserProject("test-user-project"), QuotaUser("test-quota-user"),
      UserIp("test-user-ip"), EncryptionKey::FromBinaryKey("01234567"),
      KmsKeyName("test-kms-key-name"));

  auto actual = GrpcObjectRequestParser::ToProto(request).value();
  EXPECT_THAT(actual, IsProtoEqual(expected));
}

TEST(GrpcObjectRequestParser, InsertObjectMediaRequestWithObjectMetadata) {
  auto constexpr kTextProto = R"pb(
    # See top-of-file comments for details on the magic numbers
    object_checksums: { crc32c: 0x22620404 }
  )pb";
  storage_proto::WriteObjectRequest expected;
  EXPECT_TRUE(TextFormat::ParseFromString(kTextProto, &expected));
  auto& resource = *expected.mutable_write_object_spec()->mutable_resource();
  resource = ExpectedFullObjectMetadata();
  resource.set_bucket("projects/_/buckets/test-bucket-name");
  resource.set_name("test-object-name");
  resource.set_storage_class("STANDARD");

  auto constexpr kContents = "The quick brown fox jumps over the lazy dog";

  InsertObjectMediaRequest request("test-bucket-name", "test-object-name",
                                   kContents);
  request.set_multiple_options(
      WithObjectMetadata(FullObjectMetadata().set_storage_class("STANDARD")));

  auto actual = GrpcObjectRequestParser::ToProto(request).value();
  EXPECT_THAT(actual, IsProtoEqual(expected));
}

TEST(GrpcObjectRequestParser, WriteObjectResponseSimple) {
  google::storage::v2::WriteObjectResponse input;
  ASSERT_TRUE(TextFormat::ParseFromString(
      R"pb(
        persisted_size: 123456
      )pb",
      &input));

  auto const actual = GrpcObjectRequestParser::FromProto(input, Options{}, {});
  EXPECT_EQ(actual.committed_size.value_or(0), 123456);
  EXPECT_FALSE(actual.payload.has_value());
}

TEST(GrpcObjectRequestParser, WriteObjectResponseWithResource) {
  google::storage::v2::WriteObjectResponse input;
  ASSERT_TRUE(TextFormat::ParseFromString(
      R"pb(
        resource {
          name: "test-object-name"
          bucket: "projects/_/buckets/test-bucket-name"
          size: 123456
        })pb",
      &input));

  auto const actual = GrpcObjectRequestParser::FromProto(
      input, Options{}, {{"header", "value"}, {"other-header", "other-value"}});
  EXPECT_FALSE(actual.committed_size.has_value());
  ASSERT_TRUE(actual.payload.has_value());
  EXPECT_EQ(actual.payload->name(), "test-object-name");
  EXPECT_EQ(actual.payload->bucket(), "test-bucket-name");
  EXPECT_EQ(actual.payload->size(), 123456);
  EXPECT_THAT(actual.request_metadata,
              UnorderedElementsAre(Pair("header", "value"),
                                   Pair("other-header", "other-value")));
}

TEST(GrpcObjectRequestParser, ListObjectsRequestAllFields) {
  google::storage::v2::ListObjectsRequest expected;
  ASSERT_TRUE(TextFormat::ParseFromString(
      R"pb(
        parent: "projects/_/buckets/test-bucket"
        page_size: 10
        page_token: "test-only-invalid"
        delimiter: "/"
        include_trailing_delimiter: true
        prefix: "test/prefix"
        versions: true
        lexicographic_start: "test/prefix/a"
        lexicographic_end: "test/prefix/abc"
      )pb",
      &expected));

  ListObjectsRequest req("test-bucket");
  req.set_page_token("test-only-invalid");
  req.set_multiple_options(
      MaxResults(10), Delimiter("/"), IncludeTrailingDelimiter(true),
      Prefix("test/prefix"), Versions(true), StartOffset("test/prefix/a"),
      EndOffset("test/prefix/abc"), UserProject("test-user-project"),
      QuotaUser("test-quota-user"), UserIp("test-user-ip"));

  auto const actual = GrpcObjectRequestParser::ToProto(req);
  EXPECT_THAT(actual, IsProtoEqual(expected));
}

TEST(GrpcObjectRequestParser, ListObjectsResponse) {
  google::storage::v2::ListObjectsResponse response;
  ASSERT_TRUE(TextFormat::ParseFromString(
      R"pb(
        objects { bucket: "projects/_/buckets/test-bucket" name: "object1" }
        objects { bucket: "projects/_/buckets/test-bucket" name: "object2" }
        prefixes: "prefix1/"
        prefixes: "prefix2/"
        next_page_token: "test-only-invalid-token"
      )pb",
      &response));

  auto actual = GrpcObjectRequestParser::FromProto(response, Options{});
  EXPECT_EQ(actual.next_page_token, "test-only-invalid-token");
  EXPECT_THAT(actual.prefixes, ElementsAre("prefix1/", "prefix2/"));
  std::vector<std::string> names;
  for (auto const& o : actual.items) names.push_back(o.bucket());
  EXPECT_THAT(names, ElementsAre("test-bucket", "test-bucket"));
  names.clear();
  for (auto const& o : actual.items) names.push_back(o.name());
  EXPECT_THAT(names, ElementsAre("object1", "object2"));
}

TEST(GrpcObjectRequestParser, RewriteObjectRequestAllOptions) {
  auto constexpr kTextProto = R"pb(
    destination_bucket: "projects/_/buckets/destination-bucket"
    destination_name: "destination-object"
    source_bucket: "projects/_/buckets/source-bucket"
    source_object: "source-object"
    source_generation: 7
    rewrite_token: "test-only-rewrite-token"
    destination_predefined_acl: "projectPrivate"
    if_generation_match: 1
    if_generation_not_match: 2
    if_metageneration_match: 3
    if_metageneration_not_match: 4
    if_source_generation_match: 5
    if_source_generation_not_match: 6
    if_source_metageneration_match: 7
    if_source_metageneration_not_match: 8
    max_bytes_rewritten_per_call: 123456
    copy_source_encryption_algorithm: "AES256"
    copy_source_encryption_key_bytes: "ABCDEFGH"
    # Used `/bin/echo -n "ABCDEFGH" | sha256sum` to create this magic string
    copy_source_encryption_key_sha256_bytes: "\x9a\xc2\x19\x7d\x92\x58\x25\x7b\x1a\xe8\x46\x3e\x42\x14\xe4\xcd\x0a\x57\x8b\xc1\x51\x7f\x24\x15\x92\x8b\x91\xbe\x42\x83\xfc\x48"
  )pb";
  google::storage::v2::RewriteObjectRequest expected;
  ASSERT_TRUE(TextFormat::ParseFromString(kTextProto, &expected));
  auto& destination = *expected.mutable_destination();
  destination = ExpectedFullObjectMetadata();
  // Set via the `DestinationKmsKeyName()` option.
  destination.set_kms_key("test-kms-key-name-from-option");
  destination.set_storage_class("STANDARD");
  *expected.mutable_common_object_request_params() =
      ExpectedCommonObjectRequestParams();

  RewriteObjectRequest req("source-bucket", "source-object",
                           "destination-bucket", "destination-object",
                           "test-only-rewrite-token");
  req.set_multiple_options(
      DestinationKmsKeyName("test-kms-key-name-from-option"),
      DestinationPredefinedAcl("projectPrivate"),
      EncryptionKey::FromBinaryKey("01234567"), IfGenerationMatch(1),
      IfGenerationNotMatch(2), IfMetagenerationMatch(3),
      IfMetagenerationNotMatch(4), IfSourceGenerationMatch(5),
      IfSourceGenerationNotMatch(6), IfSourceMetagenerationMatch(7),
      IfSourceMetagenerationNotMatch(8), MaxBytesRewrittenPerCall(123456),
      Projection("full"), SourceEncryptionKey::FromBinaryKey("ABCDEFGH"),
      SourceGeneration(7), UserProject("test-user-project"),
      QuotaUser("test-quota-user"), UserIp("test-user-ip"),
      WithObjectMetadata(FullObjectMetadata().set_storage_class("STANDARD")));

  auto const actual = GrpcObjectRequestParser::ToProto(req);
  ASSERT_STATUS_OK(actual);
  EXPECT_THAT(*actual, IsProtoEqual(expected));
}

TEST(GrpcObjectRequestParser, RewriteObjectRequestNoDestination) {
  google::storage::v2::RewriteObjectRequest expected;
  ASSERT_TRUE(TextFormat::ParseFromString(
      R"pb(
        destination_bucket: "projects/_/buckets/destination-bucket"
        destination_name: "destination-object"
        source_bucket: "projects/_/buckets/source-bucket"
        source_object: "source-object"
        source_generation: 7
        rewrite_token: "test-only-rewrite-token"
        destination_predefined_acl: "projectPrivate"
        if_generation_match: 1
        if_generation_not_match: 2
        if_metageneration_match: 3
        if_metageneration_not_match: 4
        if_source_generation_match: 5
        if_source_generation_not_match: 6
        if_source_metageneration_match: 7
        if_source_metageneration_not_match: 8
        max_bytes_rewritten_per_call: 123456
        copy_source_encryption_algorithm: "AES256"
        copy_source_encryption_key_bytes: "ABCDEFGH"
        # Used `/bin/echo -n "ABCDEFGH" | sha256sum` to create this magic string
        copy_source_encryption_key_sha256_bytes: "\x9a\xc2\x19\x7d\x92\x58\x25\x7b\x1a\xe8\x46\x3e\x42\x14\xe4\xcd\x0a\x57\x8b\xc1\x51\x7f\x24\x15\x92\x8b\x91\xbe\x42\x83\xfc\x48"
      )pb",
      &expected));
  *expected.mutable_common_object_request_params() =
      ExpectedCommonObjectRequestParams();

  RewriteObjectRequest req("source-bucket", "source-object",
                           "destination-bucket", "destination-object",
                           "test-only-rewrite-token");
  req.set_multiple_options(
      DestinationPredefinedAcl("projectPrivate"),
      EncryptionKey::FromBinaryKey("01234567"), IfGenerationMatch(1),
      IfGenerationNotMatch(2), IfMetagenerationMatch(3),
      IfMetagenerationNotMatch(4), IfSourceGenerationMatch(5),
      IfSourceGenerationNotMatch(6), IfSourceMetagenerationMatch(7),
      IfSourceMetagenerationNotMatch(8), MaxBytesRewrittenPerCall(123456),
      Projection("full"), SourceEncryptionKey::FromBinaryKey("ABCDEFGH"),
      SourceGeneration(7), UserProject("test-user-project"),
      QuotaUser("test-quota-user"), UserIp("test-user-ip"));

  auto const actual = GrpcObjectRequestParser::ToProto(req);
  ASSERT_STATUS_OK(actual);
  EXPECT_THAT(*actual, IsProtoEqual(expected));
}

TEST(GrpcObjectRequestParser, RewriteObjectResponse) {
  google::storage::v2::RewriteResponse input;
  ASSERT_TRUE(TextFormat::ParseFromString(
      R"pb(
        total_bytes_rewritten: 123456
        object_size: 1234560
        done: false
        rewrite_token: "test-only-token"
        resource {
          bucket: "projects/_/buckets/bucket-name"
          name: "object-name"
        }
      )pb",
      &input));

  auto const actual = GrpcObjectRequestParser::FromProto(input, Options{});
  EXPECT_EQ(actual.total_bytes_rewritten, 123456);
  EXPECT_EQ(actual.object_size, 1234560);
  EXPECT_FALSE(actual.done);
  EXPECT_EQ(actual.rewrite_token, "test-only-token");
  EXPECT_EQ(actual.resource.bucket(), "bucket-name");
  EXPECT_EQ(actual.resource.name(), "object-name");
}

TEST(GrpcObjectRequestParser, CopyObjectRequestAllOptions) {
  auto constexpr kTextProto = R"pb(
    destination_bucket: "projects/_/buckets/destination-bucket"
    destination_name: "destination-object"
    source_bucket: "projects/_/buckets/source-bucket"
    source_object: "source-object"
    source_generation: 7
    destination_predefined_acl: "projectPrivate"
    if_generation_match: 1
    if_generation_not_match: 2
    if_metageneration_match: 3
    if_metageneration_not_match: 4
    if_source_generation_match: 5
    if_source_generation_not_match: 6
    if_source_metageneration_match: 7
    if_source_metageneration_not_match: 8
    copy_source_encryption_algorithm: "AES256"
    copy_source_encryption_key_bytes: "ABCDEFGH"
    # Used `/bin/echo -n "ABCDEFGH" | sha256sum` to create this magic string
    copy_source_encryption_key_sha256_bytes: "\x9a\xc2\x19\x7d\x92\x58\x25\x7b\x1a\xe8\x46\x3e\x42\x14\xe4\xcd\x0a\x57\x8b\xc1\x51\x7f\x24\x15\x92\x8b\x91\xbe\x42\x83\xfc\x48"
  )pb";
  google::storage::v2::RewriteObjectRequest expected;
  ASSERT_TRUE(TextFormat::ParseFromString(kTextProto, &expected));
  auto& destination = *expected.mutable_destination();
  destination = ExpectedFullObjectMetadata();
  destination.set_kms_key("test-kms-key-name-from-option");
  destination.set_storage_class("STANDARD");
  *expected.mutable_common_object_request_params() =
      ExpectedCommonObjectRequestParams();

  CopyObjectRequest req("source-bucket", "source-object", "destination-bucket",
                        "destination-object");
  req.set_multiple_options(
      DestinationKmsKeyName("test-kms-key-name-from-option"),
      DestinationPredefinedAcl("projectPrivate"),
      EncryptionKey::FromBinaryKey("01234567"), IfGenerationMatch(1),
      IfGenerationNotMatch(2), IfMetagenerationMatch(3),
      IfMetagenerationNotMatch(4), IfSourceGenerationMatch(5),
      IfSourceGenerationNotMatch(6), IfSourceMetagenerationMatch(7),
      IfSourceMetagenerationNotMatch(8), Projection("full"),
      SourceEncryptionKey::FromBinaryKey("ABCDEFGH"), SourceGeneration(7),
      UserProject("test-user-project"), QuotaUser("test-quota-user"),
      UserIp("test-user-ip"),
      WithObjectMetadata(FullObjectMetadata().set_storage_class("STANDARD")));

  auto const actual = GrpcObjectRequestParser::ToProto(req);
  ASSERT_STATUS_OK(actual);
  EXPECT_THAT(*actual, IsProtoEqual(expected));
}

TEST(GrpcObjectRequestParser, CopyObjectRequestNoDestination) {
  google::storage::v2::RewriteObjectRequest expected;
  ASSERT_TRUE(TextFormat::ParseFromString(
      R"pb(
        destination_bucket: "projects/_/buckets/destination-bucket"
        destination_name: "destination-object"
        source_bucket: "projects/_/buckets/source-bucket"
        source_object: "source-object"
        source_generation: 7
        destination_predefined_acl: "projectPrivate"
        if_generation_match: 1
        if_generation_not_match: 2
        if_metageneration_match: 3
        if_metageneration_not_match: 4
        if_source_generation_match: 5
        if_source_generation_not_match: 6
        if_source_metageneration_match: 7
        if_source_metageneration_not_match: 8
        copy_source_encryption_algorithm: "AES256"
        copy_source_encryption_key_bytes: "ABCDEFGH"
        # Used `/bin/echo -n "ABCDEFGH" | sha256sum` to create this magic string
        copy_source_encryption_key_sha256_bytes: "\x9a\xc2\x19\x7d\x92\x58\x25\x7b\x1a\xe8\x46\x3e\x42\x14\xe4\xcd\x0a\x57\x8b\xc1\x51\x7f\x24\x15\x92\x8b\x91\xbe\x42\x83\xfc\x48"
      )pb",
      &expected));
  *expected.mutable_common_object_request_params() =
      ExpectedCommonObjectRequestParams();

  CopyObjectRequest req("source-bucket", "source-object", "destination-bucket",
                        "destination-object");
  req.set_multiple_options(
      DestinationPredefinedAcl("projectPrivate"),
      EncryptionKey::FromBinaryKey("01234567"), IfGenerationMatch(1),
      IfGenerationNotMatch(2), IfMetagenerationMatch(3),
      IfMetagenerationNotMatch(4), IfSourceGenerationMatch(5),
      IfSourceGenerationNotMatch(6), IfSourceMetagenerationMatch(7),
      IfSourceMetagenerationNotMatch(8), Projection("full"),
      SourceEncryptionKey::FromBinaryKey("ABCDEFGH"), SourceGeneration(7),
      UserProject("test-user-project"), QuotaUser("test-quota-user"),
      UserIp("test-user-ip"));

  auto const actual = GrpcObjectRequestParser::ToProto(req);
  ASSERT_STATUS_OK(actual);
  EXPECT_THAT(*actual, IsProtoEqual(expected));
}

TEST(GrpcObjectRequestParser, ResumableUploadRequestSimple) {
  google::storage::v2::StartResumableWriteRequest expected;
  EXPECT_TRUE(TextFormat::ParseFromString(R"""(
      write_object_spec: {
          resource: {
            name: "test-object"
            bucket: "projects/_/buckets/test-bucket"
          }
      })""",
                                          &expected));

  ResumableUploadRequest req("test-bucket", "test-object");

  auto actual = GrpcObjectRequestParser::ToProto(req).value();
  EXPECT_THAT(actual, IsProtoEqual(expected));
}

TEST(GrpcObjectRequestParser, ResumableUploadRequestAllFields) {
  google::storage::v2::StartResumableWriteRequest expected;
  EXPECT_TRUE(TextFormat::ParseFromString(
      R"pb(
        write_object_spec: {
          resource: {
            name: "test-object"
            bucket: "projects/_/buckets/test-bucket"
            content_encoding: "test-content-encoding"
            content_type: "test-content-type"
            # Should not be set, the proto file says these values should
            # not be included in the upload
            #     crc32c:
            #     md5_hash:
            kms_key: "test-kms-key-name"
          }
          predefined_acl: "private"
          if_generation_match: 0
          if_generation_not_match: 7
          if_metageneration_match: 42
          if_metageneration_not_match: 84
        }
      )pb",
      &expected));
  *expected.mutable_common_object_request_params() =
      ExpectedCommonObjectRequestParams();

  ResumableUploadRequest req("test-bucket", "test-object");
  req.set_multiple_options(
      ContentType("test-content-type"),
      ContentEncoding("test-content-encoding"),
      Crc32cChecksumValue(
          ComputeCrc32cChecksum("The quick brown fox jumps over the lazy dog")),
      MD5HashValue(
          ComputeMD5Hash("The quick brown fox jumps over the lazy dog")),
      PredefinedAcl("private"), IfGenerationMatch(0), IfGenerationNotMatch(7),
      IfMetagenerationMatch(42), IfMetagenerationNotMatch(84),
      Projection::Full(), UserProject("test-user-project"),
      QuotaUser("test-quota-user"), UserIp("test-user-ip"),
      EncryptionKey::FromBinaryKey("01234567"),
      KmsKeyName("test-kms-key-name"));

  auto actual = GrpcObjectRequestParser::ToProto(req).value();
  EXPECT_THAT(actual, IsProtoEqual(expected));
}

TEST(GrpcObjectRequestParser, ResumableUploadRequestWithObjectMetadataFields) {
  google::storage::v2::StartResumableWriteRequest expected;
  auto& resource = *expected.mutable_write_object_spec()->mutable_resource();
  resource = ExpectedFullObjectMetadata();
  // In this particular case, the object name and bucket are part of the
  // metadata
  resource.set_name("test-object");
  resource.set_bucket("projects/_/buckets/test-bucket");
  resource.set_storage_class("STANDARD");

  ResumableUploadRequest req("test-bucket", "test-object");
  req.set_multiple_options(
      WithObjectMetadata(FullObjectMetadata().set_storage_class("STANDARD")));

  auto actual = GrpcObjectRequestParser::ToProto(req).value();
  EXPECT_THAT(actual, IsProtoEqual(expected));
}

TEST(GrpcObjectRequestParser, ResumableUploadRequestWithContentLength) {
  google::storage::v2::StartResumableWriteRequest expected;
  EXPECT_TRUE(TextFormat::ParseFromString(R"""(
      write_object_spec: {
          resource: {
            name: "test-object"
            bucket: "projects/_/buckets/test-bucket"
          }
          object_size: 123456
      })""",
                                          &expected));

  ResumableUploadRequest req("test-bucket", "test-object");
  req.set_multiple_options(UploadContentLength(123456));

  auto actual = GrpcObjectRequestParser::ToProto(req).value();
  EXPECT_THAT(actual, IsProtoEqual(expected));
}

TEST(GrpcObjectRequestParser, QueryResumableUploadRequestSimple) {
  google::storage::v2::QueryWriteStatusRequest expected;
  EXPECT_TRUE(TextFormat::ParseFromString(
      R"pb(
        upload_id: "test-upload-id"
      )pb",
      &expected));

  QueryResumableUploadRequest req("test-upload-id");

  auto actual = GrpcObjectRequestParser::ToProto(req);
  EXPECT_THAT(actual, IsProtoEqual(expected));
}

TEST(GrpcObjectRequestParser, QueryResumableUploadResponseSimple) {
  google::storage::v2::QueryWriteStatusResponse input;
  ASSERT_TRUE(TextFormat::ParseFromString(
      R"pb(
        persisted_size: 123456
      )pb",
      &input));

  auto const actual = GrpcObjectRequestParser::FromProto(input, Options{});
  EXPECT_EQ(actual.committed_size.value_or(0), 123456);
  EXPECT_FALSE(actual.payload.has_value());
}

TEST(GrpcObjectRequestParser, QueryResumableUploadResponseWithResource) {
  google::storage::v2::QueryWriteStatusResponse input;
  ASSERT_TRUE(TextFormat::ParseFromString(
      R"pb(
        resource {
          name: "test-object-name"
          bucket: "projects/_/buckets/test-bucket-name"
          size: 123456
        })pb",
      &input));

  auto const actual = GrpcObjectRequestParser::FromProto(input, Options{});
  EXPECT_FALSE(actual.committed_size.has_value());
  ASSERT_TRUE(actual.payload.has_value());
  EXPECT_EQ(actual.payload->name(), "test-object-name");
  EXPECT_EQ(actual.payload->bucket(), "test-bucket-name");
  EXPECT_EQ(actual.payload->size(), 123456);
}

TEST(GrpcObjectRequestParser, DeleteResumableUploadRequest) {
  google::storage::v2::CancelResumableWriteRequest expected;
  EXPECT_TRUE(TextFormat::ParseFromString(
      R"pb(
        upload_id: "test-upload-id"
      )pb",
      &expected));

  DeleteResumableUploadRequest req("test-upload-id");

  auto actual = GrpcObjectRequestParser::ToProto(req);
  EXPECT_THAT(actual, IsProtoEqual(expected));
}

}  // namespace
}  // namespace internal
GOOGLE_CLOUD_CPP_INLINE_NAMESPACE_END
}  // namespace storage
}  // namespace cloud
}  // namespace google
