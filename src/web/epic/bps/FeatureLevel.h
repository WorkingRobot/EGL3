#pragma once

#include <stdint.h>

namespace EGL3::Web::Epic::BPS {
    enum class FeatureLevel : int32_t {
        // The original version.
        Original = 0,
        // Support for custom fields.
        CustomFields,
        // Started storing the version number.
        StartStoringVersion,
        // Made after data files where renamed to include the hash value, these chunks now go to ChunksV2.
        DataFileRenames,
        // Manifest stores whether build was constructed with chunk or file data.
        StoresIfChunkOrFileData,
        // Manifest stores group number for each chunk/file data for reference so that external readers don't need to know how to calculate them.
        StoresDataGroupNumbers,
        // Added support for chunk compression, these chunks now go to ChunksV3. NB: Not File Data Compression yet.
        ChunkCompressionSupport,
        // Manifest stores product prerequisites info.
        StoresPrerequisitesInfo,
        // Manifest stores chunk download sizes.
        StoresChunkFileSizes,
        // Manifest can optionally be stored using UObject serialization and compressed.
        StoredAsCompressedUClass,
        // These two features were removed and never used.
        UNUSED_0,
        UNUSED_1,
        // Manifest stores chunk data SHA1 hash to use in place of data compare, for faster generation.
        StoresChunkDataShaHashes,
        // Manifest stores Prerequisite Ids.
        StoresPrerequisiteIds,
        // The first minimal binary format was added. UObject classes will no longer be saved out when binary selected.
        StoredAsBinaryData,
        // Temporary level where manifest can reference chunks with dynamic window size, but did not serialize them. Chunks from here onwards are stored in ChunksV4.
        VariableSizeChunksWithoutWindowSizeChunkInfo,
        // Manifest can reference chunks with dynamic window size, and also serializes them.
        VariableSizeChunks,
        // Manifest uses a build id generated from its metadata.
        UsesRuntimeGeneratedBuildId,
        // Manifest uses a build id generated unique at build time, and stored in manifest.
        UsesBuildTimeGeneratedBuildId,

        // !! Always after the latest version entry, signifies the latest version plus 1 to allow the following Latest alias.
        LatestPlusOne,
        // An alias for the actual latest version value.
        Latest = (LatestPlusOne - 1),
        // An alias to provide the latest version of a manifest supported by file data (nochunks).
        LatestNoChunks = StoresChunkFileSizes,
        // An alias to provide the latest version of a manifest supported by a json serialized format.
        LatestJson = StoresPrerequisiteIds,
        // An alias to provide the first available version of optimised delta manifest saving.
        FirstOptimisedDelta = UsesRuntimeGeneratedBuildId,

        // More aliases, but this time for values that have been renamed
        StoresUniqueBuildId = UsesRuntimeGeneratedBuildId,

        // JSON manifests were stored with a version of 255 during a certain CL range due to a bug.
        // We will treat this as being StoresChunkFileSizes in code.
        BrokenJsonVersion = 255,
        // This is for UObject default, so that we always serialize it.
        Invalid = -1
    };
    
    static constexpr const char* GetChunkSubdir(FeatureLevel SelectedLevel) {
        switch (SelectedLevel)
        {
        case EGL3::Web::Epic::BPS::FeatureLevel::Original:
        case EGL3::Web::Epic::BPS::FeatureLevel::CustomFields:
        case EGL3::Web::Epic::BPS::FeatureLevel::StartStoringVersion:
            return "Chunks";
        case EGL3::Web::Epic::BPS::FeatureLevel::DataFileRenames:
        case EGL3::Web::Epic::BPS::FeatureLevel::StoresIfChunkOrFileData:
        case EGL3::Web::Epic::BPS::FeatureLevel::StoresDataGroupNumbers:
            return "ChunksV2";
        case EGL3::Web::Epic::BPS::FeatureLevel::ChunkCompressionSupport:
        case EGL3::Web::Epic::BPS::FeatureLevel::StoresPrerequisitesInfo:
        case EGL3::Web::Epic::BPS::FeatureLevel::StoresChunkFileSizes:
        case EGL3::Web::Epic::BPS::FeatureLevel::StoredAsCompressedUClass:
        case EGL3::Web::Epic::BPS::FeatureLevel::StoresChunkDataShaHashes:
        case EGL3::Web::Epic::BPS::FeatureLevel::StoresPrerequisiteIds:
        case EGL3::Web::Epic::BPS::FeatureLevel::StoredAsBinaryData:
            return "ChunksV3";
        case EGL3::Web::Epic::BPS::FeatureLevel::VariableSizeChunksWithoutWindowSizeChunkInfo:
        case EGL3::Web::Epic::BPS::FeatureLevel::VariableSizeChunks:
        case EGL3::Web::Epic::BPS::FeatureLevel::UsesRuntimeGeneratedBuildId:
        case EGL3::Web::Epic::BPS::FeatureLevel::UsesBuildTimeGeneratedBuildId:
            return "ChunksV4";
        default:
            return "";
        }
    }

    template<FeatureLevel SelectedLevel>
    static constexpr const char* GetChunkSubdir() {
        return GetChunkSubdir(SelectedLevel);
    }
}