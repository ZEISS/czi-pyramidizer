// SPDX-FileCopyrightText: 2024 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: MIT

#include <gtest/gtest.h>
#include "inc/pyramidizer_operation.h"

#include <array>

#include "libCZIUtilities.h"

#include <tuple>
#include <memory>
#include <limits>
#include <algorithm>

using namespace std;
using namespace libpyramidizer;
using namespace libCZI;

namespace
{
    struct TileInfo
    {
        int32_t x;
        int32_t y;
    };

    struct MosaicInfo
    {
        int tile_width;
        int tile_height;
        std::vector<TileInfo> tiles;
    };

    tuple<shared_ptr<void>, size_t> CreateMosaicCzi(const MosaicInfo& mosaic_info)
    {
        auto writer = libCZI::CreateCZIWriter();
        auto mem_output_stream = make_shared<CMemOutputStream>(0);

        auto spWriterInfo = make_shared<libCZI::CCziWriterInfo>(
            libCZI::GUID{ 0x1234567, 0x89ab, 0xcdef, {1, 2, 3, 4, 5, 6, 7, 8} },  // NOLINT
            libCZI::CDimBounds{ {libCZI::DimensionIndex::C, 0, 1} },  // set a bounds for C
            0, static_cast<int>(mosaic_info.tiles.size() - 1));  // set a bounds M : 0<=m<=0
        writer->Create(mem_output_stream, spWriterInfo);

        for (size_t i = 0; i < mosaic_info.tiles.size(); ++i)
        {
            auto bitmap = CreateGray8BitmapAndFill(mosaic_info.tile_width, mosaic_info.tile_height, static_cast<uint8_t>(i + 0x1));
            libCZI::AddSubBlockInfoStridedBitmap addSbBlkInfo;
            addSbBlkInfo.Clear();
            addSbBlkInfo.coordinate.Set(libCZI::DimensionIndex::C, 0);
            addSbBlkInfo.mIndexValid = true;
            addSbBlkInfo.mIndex = static_cast<int>(i);
            addSbBlkInfo.x = mosaic_info.tiles[i].x;
            addSbBlkInfo.y = mosaic_info.tiles[i].y;
            addSbBlkInfo.logicalWidth = static_cast<int>(bitmap->GetWidth());
            addSbBlkInfo.logicalHeight = static_cast<int>(bitmap->GetHeight());
            addSbBlkInfo.physicalWidth = static_cast<int>(bitmap->GetWidth());
            addSbBlkInfo.physicalHeight = static_cast<int>(bitmap->GetHeight());
            addSbBlkInfo.PixelType = bitmap->GetPixelType();
            {
                const libCZI::ScopedBitmapLockerSP lock_info_bitmap{ bitmap };
                addSbBlkInfo.ptrBitmap = lock_info_bitmap.ptrDataRoi;
                addSbBlkInfo.strideBitmap = lock_info_bitmap.stride;
                writer->SyncAddSubBlock(addSbBlkInfo);
            }
        }

        const libCZI::PrepareMetadataInfo prepare_metadata_info;
        auto meta_data_builder = writer->GetPreparedMetadata(prepare_metadata_info);
        meta_data_builder->GetRootNode()->GetOrCreateChildNode("Metadata/Information/Image/OriginalCompressionMethod")->SetValue("JpegXr");
        meta_data_builder->GetRootNode()
            ->GetOrCreateChildNode("Metadata/Information/Image/OriginalCompressionParameters")
            ->SetValue("Lossless: False, Quality: 77");

        // NOLINTNEXTLINE: uninitialized struct is OK b/o Clear()
        libCZI::WriteMetadataInfo write_metadata_info;
        write_metadata_info.Clear();
        const auto& strMetadata = meta_data_builder->GetXml();
        write_metadata_info.szMetadata = strMetadata.c_str();
        write_metadata_info.szMetadataSize = strMetadata.size() + 1;
        write_metadata_info.ptrAttachment = nullptr;
        write_metadata_info.attachmentSize = 0;
        writer->SyncWriteMetadata(write_metadata_info);
        writer->Close();
        writer.reset();

        size_t czi_document_size = 0;
        const shared_ptr<void> czi_document_data = mem_output_stream->GetCopy(&czi_document_size);
        return make_tuple(czi_document_data, czi_document_size);
    }

    tuple<shared_ptr<void>, size_t> CreateMosaicCziWith3ChannelsAndChannel1Empty(const MosaicInfo& mosaic_info)
    {
        auto writer = libCZI::CreateCZIWriter();
        auto mem_output_stream = make_shared<CMemOutputStream>(0);

        auto spWriterInfo = make_shared<libCZI::CCziWriterInfo>(
            libCZI::GUID{ 0x1234567, 0x89ab, 0xcdef, {1, 2, 3, 4, 5, 6, 7, 8} },  // NOLINT
            libCZI::CDimBounds{ {libCZI::DimensionIndex::C, 0, 3} },  // set a bounds for C
            0, static_cast<int>(mosaic_info.tiles.size() - 1));  // set a bounds M : 0<=m<=0
        writer->Create(mem_output_stream, spWriterInfo);

        for (int channel_no = 0; channel_no < 3; ++channel_no)
        {
            if (channel_no == 1)
            {
                continue; // skip channel 1 to make it empty
            }

            for (size_t i = 0; i < mosaic_info.tiles.size(); ++i)
            {
                auto bitmap = CreateGray8BitmapAndFill(mosaic_info.tile_width, mosaic_info.tile_height, static_cast<uint8_t>(i + 0x1));
                libCZI::AddSubBlockInfoStridedBitmap addSbBlkInfo;
                addSbBlkInfo.Clear();
                addSbBlkInfo.coordinate.Set(libCZI::DimensionIndex::C, channel_no);
                addSbBlkInfo.mIndexValid = true;
                addSbBlkInfo.mIndex = static_cast<int>(i);
                addSbBlkInfo.x = mosaic_info.tiles[i].x;
                addSbBlkInfo.y = mosaic_info.tiles[i].y;
                addSbBlkInfo.logicalWidth = static_cast<int>(bitmap->GetWidth());
                addSbBlkInfo.logicalHeight = static_cast<int>(bitmap->GetHeight());
                addSbBlkInfo.physicalWidth = static_cast<int>(bitmap->GetWidth());
                addSbBlkInfo.physicalHeight = static_cast<int>(bitmap->GetHeight());
                addSbBlkInfo.PixelType = bitmap->GetPixelType();
                {
                    const libCZI::ScopedBitmapLockerSP lock_info_bitmap{ bitmap };
                    addSbBlkInfo.ptrBitmap = lock_info_bitmap.ptrDataRoi;
                    addSbBlkInfo.strideBitmap = lock_info_bitmap.stride;
                    writer->SyncAddSubBlock(addSbBlkInfo);
                }
            }
        }

        const libCZI::PrepareMetadataInfo prepare_metadata_info;
        auto meta_data_builder = writer->GetPreparedMetadata(prepare_metadata_info);
        meta_data_builder->GetRootNode()->GetOrCreateChildNode("Metadata/Information/Image/OriginalCompressionMethod")->SetValue("JpegXr");
        meta_data_builder->GetRootNode()
            ->GetOrCreateChildNode("Metadata/Information/Image/OriginalCompressionParameters")
            ->SetValue("Lossless: False, Quality: 77");

        // NOLINTNEXTLINE: uninitialized struct is OK b/o Clear()
        libCZI::WriteMetadataInfo write_metadata_info;
        write_metadata_info.Clear();
        const auto& strMetadata = meta_data_builder->GetXml();
        write_metadata_info.szMetadata = strMetadata.c_str();
        write_metadata_info.szMetadataSize = strMetadata.size() + 1;
        write_metadata_info.ptrAttachment = nullptr;
        write_metadata_info.attachmentSize = 0;
        writer->SyncWriteMetadata(write_metadata_info);
        writer->Close();
        writer.reset();

        size_t czi_document_size = 0;
        const shared_ptr<void> czi_document_data = mem_output_stream->GetCopy(&czi_document_size);
        return make_tuple(czi_document_data, czi_document_size);
    }

    /// Creates a CZI with four subblocks of size 100x100 of pixeltype "Gray8" in a
    /// mosaic arrangement. The arrangement is as follows:
    /// +--+--+
    /// |0 |1 |
    /// |  |  |
    /// +--+--+
    /// |2 |3 |
    /// |  |  |
    /// +--+--+
    /// Subblock 0 contains the value 0x1, subblock 1 contains the value 0x2,
    /// subblock 2 contains the value 0x3 and subblock 3 contains the value 0x4.
    /// \returns    A blob containing a CZI document.
    tuple<shared_ptr<void>, size_t> CreateCziWithFourSubBlockInMosaicArrangement()
    {
        MosaicInfo mosaic_info;
        mosaic_info.tile_width = 100;
        mosaic_info.tile_height = 100;
        mosaic_info.tiles = { {0,0}, {100,0}, {0,100}, {100,100} };
        return CreateMosaicCzi(mosaic_info);
    }

    tuple<shared_ptr<void>, size_t> CreateCziWithFourSubBlockInMosaicArrangementScenario2()
    {
        MosaicInfo mosaic_info;
        mosaic_info.tile_width = 1000;
        mosaic_info.tile_height = 1000;
        mosaic_info.tiles = { {3,4}, {2110,370}, {830,1770}, {2330,2010},{999,999},{2003,1453} };
        return CreateMosaicCzi(mosaic_info);
    }

    tuple<shared_ptr<void>, size_t> CreateCziWith3ChannelsFourSubBlockInMosaicArrangementWithChannel1Empty()
    {
        MosaicInfo mosaic_info;
        mosaic_info.tile_width = 1000;
        mosaic_info.tile_height = 1000;
        mosaic_info.tiles = { {3,4}, {2110,370}, {830,1770}, {2330,2010},{999,999},{2003,1453} };
        return CreateMosaicCziWith3ChannelsAndChannel1Empty(mosaic_info);
    }

    libCZI::PyramidStatistics::PyramidLayerStatistics GetPyramidLayerStatisticsForSceneAndLayerNo(const libCZI::PyramidStatistics& pyramid_statistics, int scene_no, int layer_no)
    {
        const auto& scene_statistics = pyramid_statistics.scenePyramidStatistics.find(scene_no);
        if (scene_statistics == pyramid_statistics.scenePyramidStatistics.end())
        {
            throw runtime_error("Scene not found");
        }

        const auto& layer_statistics = scene_statistics->second;
        for (const auto& layer : layer_statistics)
        {
            if (layer.layerInfo.pyramidLayerNo == layer_no)
            {
                return layer;
            }
        }

        throw runtime_error("Layer not found");
    }
} // namespace


// --------------------------------------------------------------------------------

struct CompressionConfiguration
{
    const char* compression_parameter;
    libCZI::CompressionMode compression_mode;
};

struct CompressionConfigurationFixture : public testing::TestWithParam<CompressionConfiguration> {};

TEST_P(CompressionConfigurationFixture, Scenario1)
{
    const CompressionConfiguration& configuration = GetParam();

    // arrange

    // call a helper function which creates a simple synthetic document in memory
    // (we get a pointer to the memory and the size of the memory)
    auto czi_document_as_blob = CreateCziWithFourSubBlockInMosaicArrangement();

    // create a stream object from this memory
    const auto memory_stream = make_shared<CMemInputOutputStream>(std::get<0>(czi_document_as_blob).get(), std::get<1>(czi_document_as_blob));

    // create a CZI-writer object on a new memory stream
    auto writer = libCZI::CreateCZIWriter();
    const auto memory_backed_stream_destination_document = make_shared<CMemInputOutputStream>(0);

    constexpr int argc = 13;
    array<const char*, argc> argv =
    {
        "czi-pyramidizer.exe",
        "-s",
        "source.czi",
        "-d",
        "destination.czi",
        "--compressionopts",
        configuration.compression_parameter,
        "--max_top_level_pyramid_size",
        "50",
        "--tile_size",
        "50",
        "--mode-of-operation",
        "Always"
    };

    CommandLineOptions command_line_options;
    const auto result = command_line_options.Parse(argc, const_cast<char**>(argv.data()));
    ASSERT_EQ(result, CommandLineOptions::ParseResult::OK);

    // act
    PyramidizerOperation::PyramidizerOperationOptions pyramidizer_operation_options;
    pyramidizer_operation_options.command_line_options = &command_line_options;
    pyramidizer_operation_options.source_stream = memory_stream;
    pyramidizer_operation_options.get_destination_stream_functor = [memory_backed_stream_destination_document]() { return memory_backed_stream_destination_document; };

    PyramidizerOperation pyramidizer_operation(pyramidizer_operation_options);
    pyramidizer_operation.Execute();

    // assert
    // 
    // and now, since the stream we used for the writer is a
    // read-write-memory-stream, we can pass the same
    // stream object to a new CZI-reader object, which we can then use to check
    // if the result is as we expect
    const auto reader_processed_document = libCZI::CreateCZIReader();
    reader_processed_document->Open(memory_backed_stream_destination_document);

    const auto statistics = reader_processed_document->GetStatistics();
    const auto pyramid_statistics = reader_processed_document->GetPyramidStatistics();

    // we expect the following:
    // - the four original sub-blocks (at position 0,0; 100,0; 0,100; 100,100)
    // - four sub-blocks at pyramid-level 1 (at the same logical positions, but with half the pixels)
    // - one top-level pyramid sub-block 

    ASSERT_EQ(statistics.subBlockCount, 9);

    // get the pyramid-statistics - since we don't use the S-index, we get the statistics for key "numeric_limits<int>::max()", 
    //  which is a magic value for the case "no S-index used"
    const auto& statistics_iterator = pyramid_statistics.scenePyramidStatistics.find(numeric_limits<int>::max());
    ASSERT_NE(statistics_iterator, pyramid_statistics.scenePyramidStatistics.end());
    const vector<PyramidStatistics::PyramidLayerStatistics>& pyramid_layer_statistics = statistics_iterator->second;

    // we expect three pyramid-levels
    ASSERT_EQ(pyramid_layer_statistics.size(), 3);
    ASSERT_EQ(pyramid_layer_statistics.at(0).count, 4);
    ASSERT_EQ(pyramid_layer_statistics.at(0).layerInfo.IsLayer0(), true);
    ASSERT_EQ(pyramid_layer_statistics.at(0).layerInfo.pyramidLayerNo, 0);
    ASSERT_EQ(pyramid_layer_statistics.at(0).layerInfo.minificationFactor, 0);
    ASSERT_EQ(pyramid_layer_statistics.at(1).count, 4);
    ASSERT_EQ(pyramid_layer_statistics.at(1).layerInfo.IsLayer0(), false);
    ASSERT_EQ(pyramid_layer_statistics.at(1).layerInfo.pyramidLayerNo, 1);
    ASSERT_EQ(pyramid_layer_statistics.at(1).layerInfo.minificationFactor, 2);
    ASSERT_EQ(pyramid_layer_statistics.at(2).count, 1);
    ASSERT_EQ(pyramid_layer_statistics.at(2).layerInfo.IsLayer0(), false);
    ASSERT_EQ(pyramid_layer_statistics.at(2).layerInfo.pyramidLayerNo, 2);
    ASSERT_EQ(pyramid_layer_statistics.at(2).layerInfo.minificationFactor, 2);

    // Now, we superficially check the content of the top-level pyramid sub-block. The layer-0 have the values 0x1, 0x2, 0x3, 0x4
    // (in the order top-left, top-right, bottom-left, bottom-right). We expect to find those values in the top-level pyramid sub-block.

    optional<int> index_of_top_level_sub_block;

    // find the top-level sub-block
    reader_processed_document->EnumerateSubBlocks(
        [&](int index, const SubBlockInfo& info)->bool
        {
            const float zoom = Utils::CalcZoom(info.logicalRect, info.physicalSize);
            if (fabs(zoom - 0.25f) < 0.0001f)
            {
                index_of_top_level_sub_block = index;
                return false;
            }

            return true;
        });

    ASSERT_TRUE(index_of_top_level_sub_block.has_value());

    auto sub_block = reader_processed_document->ReadSubBlock(index_of_top_level_sub_block.value());
    ASSERT_EQ(sub_block->GetSubBlockInfo().GetCompressionMode(), configuration.compression_mode);

    auto bitmap = sub_block->CreateBitmap();

    ASSERT_EQ(bitmap->GetPixelType(), PixelType::Gray8);
    ASSERT_EQ(bitmap->GetWidth(), 50);
    ASSERT_EQ(bitmap->GetHeight(), 50);

    const auto lock = ScopedBitmapLockerSP(bitmap);

    // now look at the four pixels at (12,12), (37,12), (12,37), (37,37)
    const uint8_t pixel_top_left_quadrant = *(static_cast<const uint8_t*>(lock.ptrDataRoi) + 12 * static_cast<size_t>(lock.stride) + 12);
    const uint8_t pixel_top_right_quadrant = *(static_cast<const uint8_t*>(lock.ptrDataRoi) + 12 * static_cast<size_t>(lock.stride) + 37);
    const uint8_t pixel_bottom_left_quadrant = *(static_cast<const uint8_t*>(lock.ptrDataRoi) + 37 * static_cast<size_t>(lock.stride) + 12);
    const uint8_t pixel_bottom_right_quadrant = *(static_cast<const uint8_t*>(lock.ptrDataRoi) + 37 * static_cast<size_t>(lock.stride) + 37);

    ASSERT_EQ(pixel_top_left_quadrant, 0x1);
    ASSERT_EQ(pixel_top_right_quadrant, 0x2);
    ASSERT_EQ(pixel_bottom_left_quadrant, 0x3);
    ASSERT_EQ(pixel_bottom_right_quadrant, 0x4);
}

INSTANTIATE_TEST_SUITE_P(
    PyramidizerOperation,
    CompressionConfigurationFixture,
    testing::Values(
    CompressionConfiguration{ "uncompressed:", CompressionMode::UnCompressed },
    CompressionConfiguration{ "zstd1:ExplicitLevel=2;PreProcess=HiLoByteUnpack", CompressionMode::Zstd1 },
    CompressionConfiguration{ "zstd0:ExplicitLevel=0", CompressionMode::Zstd0 },
    CompressionConfiguration{ "jpgxr:", CompressionMode::JpgXr }),
    [](const testing::TestParamInfo<CompressionConfigurationFixture::ParamType>& info)
    {
        // Can use info.param here to generate the test suffix
        return string{ libCZI::Utils::CompressionModeToInformalString(info.param.compression_mode) };
    }
);

TEST(PyramidizerOperation, InvalidInitOptionsExpectException)
{
    PyramidizerOperation::PyramidizerOperationOptions options{};

    ASSERT_THROW(PyramidizerOperation{ options }, invalid_argument);
}

TEST(PyramidizerOperation, TestSyntheticDocumentAndCheckResult)
{
    // call a helper function which creates a simple synthetic document in memory
    // (we get a pointer to the memory and the size of the memory)
    auto czi_document_as_blob = CreateCziWithFourSubBlockInMosaicArrangementScenario2();

    // create a stream object from this memory
    const auto memory_stream = make_shared<CMemInputOutputStream>(std::get<0>(czi_document_as_blob).get(), std::get<1>(czi_document_as_blob));

    // create a CZI-writer object on a new memory stream
    auto writer = libCZI::CreateCZIWriter();
    auto memory_backed_stream_destination_document = make_shared<CMemInputOutputStream>(0);

    constexpr int argc = 15;
    std::array<const char*, argc> argv =
    {
        "czi-pyramidizer.exe",
        "-s",
        "source.czi",
        "-d",
        "destination.czi",
        "--compressionopts",
        "zstd1:",
        "--max_top_level_pyramid_size",
        "64",
        "--tile_size",
        "512",
        "--background-color",
        "white",
        "--mode-of-operation",
        "Always"
    };

    CommandLineOptions command_line_options;
    const auto result = command_line_options.Parse(argc, const_cast<char**>(argv.data()));
    ASSERT_EQ(result, CommandLineOptions::ParseResult::OK);

    // act
    PyramidizerOperation::PyramidizerOperationOptions pyramidizer_operation_options;
    pyramidizer_operation_options.command_line_options = &command_line_options;
    pyramidizer_operation_options.source_stream = memory_stream;
    pyramidizer_operation_options.get_destination_stream_functor = [memory_backed_stream_destination_document]() { return memory_backed_stream_destination_document; };

    PyramidizerOperation pyramidizer_operation(pyramidizer_operation_options);
    pyramidizer_operation.Execute();

    /*
    // for debugging purposes, write the result to a CZI-file
    FILE* fp = fopen("D:\\test_new.czi", "wb");
    fwrite(memory_backed_stream_destination_document->GetDataC(), 1, memory_backed_stream_destination_document->GetDataSize(), fp);
    fclose(fp);*/

    // assert
    const auto reader_processed_document = libCZI::CreateCZIReader();
    reader_processed_document->Open(memory_backed_stream_destination_document);

    const auto statistics = reader_processed_document->GetStatistics();
    const auto pyramid_statistics = reader_processed_document->GetPyramidStatistics();

    // we expect to find 23 subblocks altogether: 6 on layer-0, 12 on layer-1, 4 on layer-2 and 1 on layer-3
    ASSERT_EQ(statistics.subBlockCount, 23);
    // Note: numeric_limits<int>::max() is to be used in case of "no s-index used"
    ASSERT_EQ(GetPyramidLayerStatisticsForSceneAndLayerNo(pyramid_statistics, numeric_limits<int>::max(), 0).count, 6);
    ASSERT_EQ(GetPyramidLayerStatisticsForSceneAndLayerNo(pyramid_statistics, numeric_limits<int>::max(), 1).count, 12);
    ASSERT_EQ(GetPyramidLayerStatisticsForSceneAndLayerNo(pyramid_statistics, numeric_limits<int>::max(), 2).count, 4);
    ASSERT_EQ(GetPyramidLayerStatisticsForSceneAndLayerNo(pyramid_statistics, numeric_limits<int>::max(), 3).count, 1);

    // find the top-level pyramid-tile
    int index_of_top_level_sub_block = -1;
    reader_processed_document->EnumerateSubBlocks(
        [&](int index, const SubBlockInfo& info)->bool
        {
            const float zoom = Utils::CalcZoom(info.logicalRect, info.physicalSize);
            if (fabs(zoom - 0.125f) < 0.0001f)
            {
                index_of_top_level_sub_block = index;
                return false;
            }

            return true;
        });

    ASSERT_NE(index_of_top_level_sub_block, -1);

    auto top_level_subblock = reader_processed_document->ReadSubBlock(index_of_top_level_sub_block);
    auto top_level_bitmap = top_level_subblock->CreateBitmap();
    ASSERT_EQ(top_level_bitmap->GetPixelType(), PixelType::Gray8);
    ASSERT_EQ(top_level_bitmap->GetWidth(), 417);
    ASSERT_EQ(top_level_bitmap->GetHeight(), 377);

    array<uint8_t, 16> hash;
    Utils::CalcMd5SumHash(top_level_bitmap.get(), hash.data(), hash.size());

    static const uint8_t expected_hash[16] = { 0x1e,0xee,0x92,0x05,0x36,0xc3,0x14,0x3e,0x71,0x10,0x7e,0x16,0x27,0x6b,0x06,0x32 };
    ASSERT_TRUE(equal(hash.begin(), hash.end(), expected_hash)) << "The hash of the bitmap of the top-level tile is different than expected.";
}

TEST(PyramidizerOperation, Test3ChannelDocumentWhereOneChannelIsEmptyAndCheckResult)
{
    // call a helper function which creates a simple synthetic document in memory
    // (we get a pointer to the memory and the size of the memory)
    auto czi_document_as_blob = CreateCziWith3ChannelsFourSubBlockInMosaicArrangementWithChannel1Empty();

    // create a stream object from this memory
    const auto memory_stream = make_shared<CMemInputOutputStream>(std::get<0>(czi_document_as_blob).get(), std::get<1>(czi_document_as_blob));

    // create a CZI-writer object on a new memory stream
    auto writer = libCZI::CreateCZIWriter();
    auto memory_backed_stream_destination_document = make_shared<CMemInputOutputStream>(0);

    constexpr int argc = 15;
    std::array<const char*, argc> argv =
    {
        "czi-pyramidizer.exe",
        "-s",
        "source.czi",
        "-d",
        "destination.czi",
        "--compressionopts",
        "zstd1:",
        "--max_top_level_pyramid_size",
        "64",
        "--tile_size",
        "512",
        "--background-color",
        "white",
        "--mode-of-operation",
        "Always"
    };

    CommandLineOptions command_line_options;
    const auto result = command_line_options.Parse(argc, const_cast<char**>(argv.data()));
    ASSERT_EQ(result, CommandLineOptions::ParseResult::OK);

    // act
    PyramidizerOperation::PyramidizerOperationOptions pyramidizer_operation_options;
    pyramidizer_operation_options.command_line_options = &command_line_options;
    pyramidizer_operation_options.source_stream = memory_stream;
    pyramidizer_operation_options.get_destination_stream_functor = [memory_backed_stream_destination_document]() { return memory_backed_stream_destination_document; };

    PyramidizerOperation pyramidizer_operation(pyramidizer_operation_options);
    pyramidizer_operation.Execute();

    /*
    // for debugging purposes, write the result to a CZI-file
    FILE* fp = fopen("D:\\test_new.czi", "wb");
    fwrite(memory_backed_stream_destination_document->GetDataC(), 1, memory_backed_stream_destination_document->GetDataSize(), fp);
    fclose(fp);*/

    // assert
    const auto reader_processed_document = libCZI::CreateCZIReader();
    reader_processed_document->Open(memory_backed_stream_destination_document);

    const auto statistics = reader_processed_document->GetStatistics();
    const auto pyramid_statistics = reader_processed_document->GetPyramidStatistics();

    // we expect to find 23 * 2 subblocks altogether: 6 on layer-0, 12 on layer-1, 4 on layer-2 and 1 on layer-3 (and this in 2 channels)
    ASSERT_EQ(statistics.subBlockCount, 46);
    // Note: numeric_limits<int>::max() is to be used in case of "no s-index used"
    ASSERT_EQ(GetPyramidLayerStatisticsForSceneAndLayerNo(pyramid_statistics, numeric_limits<int>::max(), 0).count, 2 * 6);
    ASSERT_EQ(GetPyramidLayerStatisticsForSceneAndLayerNo(pyramid_statistics, numeric_limits<int>::max(), 1).count, 2 * 12);
    ASSERT_EQ(GetPyramidLayerStatisticsForSceneAndLayerNo(pyramid_statistics, numeric_limits<int>::max(), 2).count, 2 * 4);
    ASSERT_EQ(GetPyramidLayerStatisticsForSceneAndLayerNo(pyramid_statistics, numeric_limits<int>::max(), 3).count, 2 * 1);

    // Now, we check if there is any sub-block in channel 1. We expect that there is none, since channel 1 was empty in the
    // input document. If there is at least one sub-block with channel-coordinate 1, then this means that the pyramidizer
    // operation created a sub-block in channel 1, which should not happen.
    CDimCoordinate plane_coordinate_channel1{ {DimensionIndex::C, 1} };
    bool sub_block_in_channel1_found = false;
    reader_processed_document->EnumSubset(&plane_coordinate_channel1, nullptr, false,
        [&sub_block_in_channel1_found](int, const SubBlockInfo&)->bool
        {
            sub_block_in_channel1_found = true;
            return true;
        });

    ASSERT_FALSE(sub_block_in_channel1_found) << "We expect that there is no sub-block in channel 1, since this channel was empty in the input document. However, at least one sub-block with channel-coordinate 1 was found.";
}
