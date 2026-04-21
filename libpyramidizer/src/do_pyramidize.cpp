// SPDX-FileCopyrightText: 2024 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: MIT

#include "../inc/do_pyramidize.h"
#include "../inc/plane_enumerator.h"
#include "../inc/tile_helper.h"
#include "../inc/tiling_enumerator.h"
#include "../inc/IContext.h"
#include "../inc/IDecimationProcessing.h"
#include "../inc/pyramid_utilities.h"

#include <libCZI.h>
#include <gsl/gsl>

#include <utility>

#include "inc/UpperPyramidLayersPyramidizer.h"

using namespace std;
using namespace libCZI;
using namespace libpyramidizer;

/// Following the "Pimpl" idiom, the implementation of the DoPyramidize class is in the Impl class.
struct DoPyramidize::Impl final
{
    explicit Impl(gsl::not_null<DoPyramidize*> parent)
        : parent_(parent)
    {
    }

    Impl() = delete;
    ~Impl() = default;
    Impl(const Impl&) = delete;
    Impl& operator=(const Impl&) = delete;
    Impl(Impl&&) = delete;
    Impl& operator=(Impl&&) = delete;

    /// Process the specified plane.
    ///
    /// \param  pyramid_region  The pyramid region to operate on.
    ///
    /// \returns    A structure containing information about the operation.
    DoPyramidize::PyramidRegionInfo DoPlane(const PlaneEnumerator::PyramidRegion& pyramid_region);

private:
    gsl::not_null<DoPyramidize*> parent_;

    TileHelper ConstructTileHelper(const std::vector<int>& sub_blocks_of_plane) const;
    std::shared_ptr<libCZI::IBitmapData> GeneratePyramidTile(const libCZI::IntRect& roi, const PlaneEnumerator::PyramidRegion& pyramid_region, const std::shared_ptr<libCZI::ISingleChannelTileAccessor>& tile_accessor);
    void WritePyramidOutputTile(const std::shared_ptr<libCZI::IBitmapData>& pyramid_tile, const libCZI::CDimCoordinate& coordinate, const libCZI::IntRect& roi);
    std::vector<int> GetSubBlockIndicesForPlane(const PlaneEnumerator::PyramidRegion& pyramid_region) const;

    /// Copies the specified sub blocks from source to destination
    ///
    /// \param  pyramid_region      The pyramid region object (only used to report the plane coordinate).
    /// \param  sub_blocks_of_plane The sub blocks of the plane.
    void CopySubBlocksToDestination(const PlaneEnumerator::PyramidRegion& pyramid_region, const std::vector<int>& sub_blocks_of_plane);

    void DoLayer0(const PlaneEnumerator::PyramidRegion& pyramid_region, const std::function<void(const std::shared_ptr<libCZI::IBitmapData>&, const libCZI::IntRect&)>& report_tile_functor);

    /// Check whether the specified rectangle is larger than the "threshold" of pyramid-generation. In other words - determine whether
    /// an additional pyramid-layer is needed for a rectangle of this size.
    ///
    /// \param  rect    The rectangle.
    ///
    /// \returns    True if above threshold, false if not.
    bool IsAbovePyramidThreshold(const libCZI::IntRect& rect) const;

    /// Check whether the specified width and height is larger than the "threshold" of pyramid-generation. In other words - determine whether
    /// an additional pyramid-layer is needed for a rectangle of this size.
    ///
    /// \param  width    The width in pixels.
    /// \param  height   The height in pixels.
    ///
    /// \returns    True if above threshold, false if not.
    bool IsAbovePyramidThreshold(std::uint32_t width, std::uint32_t height) const;

    static libCZI::CDimCoordinate FilterOutSceneIndex(const libCZI::CDimCoordinate& coordinate);

    void WritePyramidOutputTileUncompressed(const std::shared_ptr<libCZI::IBitmapData>& pyramid_tile, const libCZI::CDimCoordinate& coordinate, const libCZI::IntRect& roi);
    void WritePyramidOutputTileZstd1Compressed(const std::shared_ptr<libCZI::IBitmapData>& pyramid_tile, const libCZI::CDimCoordinate& coordinate, const libCZI::IntRect& roi, const ICompressParameters* compression_parameters);
    void WritePyramidOutputTileZstd0Compressed(const std::shared_ptr<libCZI::IBitmapData>& pyramid_tile, const libCZI::CDimCoordinate& coordinate, const libCZI::IntRect& roi, const ICompressParameters* compression_parameters);
    void WritePyramidOutputTileJpgXrCompressed(const std::shared_ptr<libCZI::IBitmapData>& pyramid_tile, const libCZI::CDimCoordinate& coordinate, const libCZI::IntRect& roi, const ICompressParameters* compression_parameters);

    /// A utility class - implementing the libCZI-IIndexSet interface, in order to restrict to a single scene.
    class IndexSetOnSingleScene : public IIndexSet
    {
        int scene_index_;
    public:
        IndexSetOnSingleScene(int scene_index)
            : scene_index_(scene_index)
        {
        }

        bool IsContained(int sceneIndex) const override
        {
            return sceneIndex == this->scene_index_;
        }
    };
};

DoPyramidize::DoPyramidize()
    : impl_(std::make_unique<Impl>(this))
{
}

DoPyramidize::~DoPyramidize()
{
}

void DoPyramidize::Initialize(const PyramidizeOptions& options)
{
    if (options.reader == nullptr)
    {
        throw invalid_argument("Reader is not set");
    }

    if (options.writer == nullptr)
    {
        throw invalid_argument("Writer is not set");
    }

    if (options.context == nullptr)
    {
        throw invalid_argument("Context is not set");
    }

    this->options_ = options;
}

DoPyramidize::PyramidizeResult DoPyramidize::Execute()
{
    this->ThrowIfNotInitialized();

    PyramidizeResult result;

    // construct a plane-iterator
    PlaneEnumerator plane_enumerator(this->options_.reader->GetStatistics());

    // iterate over all planes (or, more exactly, over all "pyramid-regions")
    for (const auto& pyramid_region : plane_enumerator)
    {
        const auto pyramid_info = this->impl_->DoPlane(pyramid_region);

        // for the "result", we are only interested in "per-scene" information, 
        //  so we consolidate the information here. We use the maximum number of pyramid-layers for a given scene.
        int s_index = numeric_limits<int>::min();
        if (pyramid_region.ContainsSceneIndex())
        {
            pyramid_region.plane_coordinate.TryGetPosition(DimensionIndex::S, &s_index);
        }

        auto existing_pyramid_info = result.pyramid_region_info.find(s_index);
        if (existing_pyramid_info != result.pyramid_region_info.end())
        {
            const uint32_t no_of_pyramid_layers = existing_pyramid_info->second.number_of_pyramid_layers;
            existing_pyramid_info->second.number_of_pyramid_layers = max(no_of_pyramid_layers, pyramid_info.number_of_pyramid_layers);
        }
        else
        {
            result.pyramid_region_info[s_index] = pyramid_info;
        }
    }

    return result;
}

void DoPyramidize::ThrowIfNotInitialized() const
{
    if (this->options_.reader == nullptr || this->options_.writer == nullptr || this->options_.context == nullptr)
    {
        throw logic_error("Not initialized");
    }
}

//----

class TileShim : public UpperPyramidLayersPyramidizer::ITile
{
private:
    shared_ptr<IBitmapData> bitmap_data_;
    IntRect rect_;
public:
    TileShim(shared_ptr<IBitmapData> bitmap_data, IntRect rect)
        : bitmap_data_(std::move(bitmap_data)), rect_(rect)
    {
    }

    libCZI::IntRect GetRect() const override
    {
        return this->rect_;
    }

    std::shared_ptr<libCZI::IBitmapData> GetBitmapData() const override
    {
        return this->bitmap_data_;
    }
};

DoPyramidize::PyramidRegionInfo DoPyramidize::Impl::DoPlane(const PlaneEnumerator::PyramidRegion& pyramid_region)
{
    PyramidRegionInfo pyramid_region_info;
    if (!this->IsAbovePyramidThreshold(pyramid_region.rect))
    {
        if (auto progress_report = this->parent_->options_.context->GetProgressReporter())
        {
            progress_report->ReportProgressPyramidizer(
                pyramid_region.plane_coordinate,
                0,  // layer
                0,  // total_count_of_tiles
                0); // count_of_tiles_done
        }

        // nothing to do in this case - but we still have to copy layer-0
        this->CopySubBlocksToDestination(pyramid_region, this->GetSubBlockIndicesForPlane(pyramid_region));
        return pyramid_region_info;
    }

    // check whether we need to have a pyramid layer above the pyramid layer we are about to create here.
    const bool is_additional_pyramid_layer_needed = this->IsAbovePyramidThreshold(pyramid_region.rect.w / 2, pyramid_region.rect.h / 2);

    if (!is_additional_pyramid_layer_needed)
    {
        bool at_least_one_pyramid_tile = false;

        // If we only need one layer, we do not have to keep the tiles of the next layer (in order to create the next layer).
        this->DoLayer0(
            pyramid_region,
            [&at_least_one_pyramid_tile](const std::shared_ptr<libCZI::IBitmapData>&, const libCZI::IntRect&)->void
            {
                at_least_one_pyramid_tile = true;
            });

        if (at_least_one_pyramid_tile)
        {
            pyramid_region_info.number_of_pyramid_layers = 1;
        }
    }
    else
    {
        std::vector<std::shared_ptr<UpperPyramidLayersPyramidizer::ITile>> next_layer_tiles;
        for (uint32_t layer = 0; ; ++layer)
        {
            if (layer == 0)
            {
                this->DoLayer0(
                    pyramid_region,
                    [&](const std::shared_ptr<libCZI::IBitmapData>& pyramid_tile, const libCZI::IntRect& rect)->void
                    {
                        next_layer_tiles.emplace_back(make_shared<TileShim>(pyramid_tile, rect));
                    });
                if (next_layer_tiles.empty())
                {
                    // this can happen if there are no valid tiles in the layer-0 (e.g. if the image is very sparse). 
                    // In this case, we do not have to create any more layers, and we are done
                    break;
                }

                pyramid_region_info.number_of_pyramid_layers = 1;
            }
            else
            {
                if (!this->IsAbovePyramidThreshold(pyramid_region.rect.w / (1 << layer), pyramid_region.rect.h / (1 << layer)))
                {
                    break;
                }

                UpperPyramidLayersPyramidizer::PyramidizerOptions upper_layer_pyramidizer_options;
                upper_layer_pyramidizer_options.context = this->parent_->options_.context;
                upper_layer_pyramidizer_options.region = pyramid_region.rect;
                upper_layer_pyramidizer_options.pyramid_layer = gsl::narrow<uint8_t>(1 + layer);
                upper_layer_pyramidizer_options.tiles = std::move(next_layer_tiles);

                // Clear the next-layer-tiles, as we are going to fill it with the next layer - it is in an undefined state
                // after the move above.
                next_layer_tiles.clear();

                // curry in the plane-coordinate
                upper_layer_pyramidizer_options.output_tile_functor =
                    [&, this](const std::shared_ptr<libCZI::IBitmapData>& pyramid_tile, const libCZI::IntRect& rect)->void
                    {
                        this->WritePyramidOutputTile(pyramid_tile, pyramid_region.plane_coordinate, rect);
                        next_layer_tiles.emplace_back(make_shared<TileShim>(pyramid_tile, rect));
                    };
                upper_layer_pyramidizer_options.progress_report =
                    [&, this](std::uint32_t total_count_of_tiles, std::uint32_t count_of_tiles_done)->void
                    {
                        if (auto progress_report = this->parent_->options_.context->GetProgressReporter())
                        {
                            progress_report->ReportProgressPyramidizer(
                                pyramid_region.plane_coordinate,
                                gsl::narrow<uint8_t>(layer),    // layer
                                total_count_of_tiles,           // total_count_of_tiles
                                count_of_tiles_done);           // count_of_tiles_done
                        }
                    };

                UpperPyramidLayersPyramidizer upper_layers_pyramidizer(upper_layer_pyramidizer_options);
                upper_layers_pyramidizer.Execute();
                if (next_layer_tiles.empty())
                {
                    // this can happen if there are no valid tiles in the layer (e.g. if the image is very sparse). 
                    // In this case, we do not have to create any more layers, and we are done
                    break;
                }

                ++pyramid_region_info.number_of_pyramid_layers;
            }
        }
    }

    return pyramid_region_info;
}

void DoPyramidize::Impl::DoLayer0(const PlaneEnumerator::PyramidRegion& pyramid_region, const std::function<void(const std::shared_ptr<libCZI::IBitmapData>&, const libCZI::IntRect&)>& report_tile_functor)
{
    const auto accessor = this->parent_->options_.reader->CreateSingleChannelTileAccessor();

    // get list of all sub-blocks in this plane/region
    const auto sub_blocks_of_plane = this->GetSubBlockIndicesForPlane(pyramid_region);

    // copy the sub-blocks to the destination document
    this->CopySubBlocksToDestination(pyramid_region, sub_blocks_of_plane);

    TileHelper tile_helper = this->ConstructTileHelper(sub_blocks_of_plane);

    TilingEnumerator tiling_enumerator(
        PyramidUtilities::EnlargeAndLatch(pyramid_region.rect, 2),
        2 * this->parent_->options_.context->GetCommandLineOptions()->GetPyramidTileWidth(),
        2 * this->parent_->options_.context->GetCommandLineOptions()->GetPyramidTileHeight());

    // report progress
    const uint32_t total_count_of_tiles = tiling_enumerator.GetCountOfTiles();
    if (auto progress_report = this->parent_->options_.context->GetProgressReporter())
    {
        progress_report->ReportProgressPyramidizer(
            pyramid_region.plane_coordinate,
            0,  // layer
            total_count_of_tiles,  // total_count_of_tiles
            0); // count_of_tiles_done
    }

    uint32_t count_of_tiles_done = 1;
    for (const auto tile_rectangle : tiling_enumerator)
    {
        auto minimal_tile = tile_helper.GetMinimalRectForRoi(tile_rectangle);
        if (minimal_tile.IsNonEmpty())
        {
            const auto minimal_tile_latched = PyramidUtilities::EnlargeAndLatch(minimal_tile, 2);

            // if the tile is too small, we discard it (and we get a nullptr here)
            if (auto pyramid_tile = this->GeneratePyramidTile(minimal_tile_latched, pyramid_region, accessor))
            {
                // and write it to the destination document
                this->WritePyramidOutputTile(pyramid_tile, pyramid_region.plane_coordinate, minimal_tile_latched);

                if (report_tile_functor)
                {
                    report_tile_functor(pyramid_tile, minimal_tile_latched);
                }
            }
        }

        if (auto progress_report = this->parent_->options_.context->GetProgressReporter())
        {
            progress_report->ReportProgressPyramidizer(
                pyramid_region.plane_coordinate,
                0,                      // layer
                total_count_of_tiles,   // total_count_of_tiles
                count_of_tiles_done);   // count_of_tiles_done
        }

        ++count_of_tiles_done;
    }
}

void DoPyramidize::Impl::CopySubBlocksToDestination(const PlaneEnumerator::PyramidRegion& pyramid_region, const std::vector<int>& sub_blocks_of_plane)
{
    uint32_t total_count_of_sub_blocks = static_cast<uint32_t>(sub_blocks_of_plane.size());
    if (auto progress_report = this->parent_->options_.context->GetProgressReporter())
    {
        progress_report->ReportProgressCopySubBlock(
            pyramid_region.plane_coordinate,
            total_count_of_sub_blocks,
            0);
    }

    uint32_t count_of_sub_blocks_done = 1;
    for (const auto sub_block_index : sub_blocks_of_plane)
    {
        auto sub_block = this->parent_->options_.reader->ReadSubBlock(sub_block_index);
        libCZI::AddSubBlockInfoMemPtr add_sub_block_info;
        add_sub_block_info.Clear();

        size_t size_data = 0;
        const std::shared_ptr<const void> rawData = sub_block->GetRawData(libCZI::ISubBlock::MemBlkType::Data, &size_data);
        add_sub_block_info.ptrData = rawData.get();
        add_sub_block_info.dataSize = gsl::narrow<uint32_t>(size_data);

        size_t metadata_size = 0;
        const std::shared_ptr<const void> spMetadata = sub_block->GetRawData(libCZI::ISubBlock::MemBlkType::Metadata, &metadata_size);
        add_sub_block_info.ptrSbBlkMetadata = spMetadata.get();
        add_sub_block_info.sbBlkMetadataSize = gsl::narrow<uint32_t>(metadata_size);

        size_t attachment_size = 0;
        const std::shared_ptr<const void> spAttachment = sub_block->GetRawData(libCZI::ISubBlock::MemBlkType::Attachment, &attachment_size);
        add_sub_block_info.ptrSbBlkAttachment = spAttachment.get();
        add_sub_block_info.sbBlkAttachmentSize = gsl::narrow<uint32_t>(attachment_size);

        const libCZI::SubBlockInfo sub_block_info = sub_block->GetSubBlockInfo();
        add_sub_block_info.coordinate = sub_block_info.coordinate;
        add_sub_block_info.x = sub_block_info.logicalRect.x;
        add_sub_block_info.y = sub_block_info.logicalRect.y;
        add_sub_block_info.logicalWidth = sub_block_info.logicalRect.w;
        add_sub_block_info.logicalHeight = sub_block_info.logicalRect.h;
        add_sub_block_info.physicalWidth = gsl::narrow<int>(sub_block_info.physicalSize.w);
        add_sub_block_info.physicalHeight = gsl::narrow<int>(sub_block_info.physicalSize.h);
        add_sub_block_info.PixelType = sub_block_info.pixelType;
        add_sub_block_info.pyramid_type = sub_block_info.pyramidType;

        if (gsl::narrow<int>(sub_block_info.physicalSize.w) == sub_block_info.logicalRect.w &&
            gsl::narrow<int>(sub_block_info.physicalSize.h) == sub_block_info.logicalRect.h &&
            sub_block_info.mIndex != (std::numeric_limits<int>::max)())
        {
            add_sub_block_info.mIndex = sub_block_info.mIndex;
            add_sub_block_info.mIndexValid = true;
        }
        else
        {
            add_sub_block_info.mIndex = 0;
            add_sub_block_info.mIndexValid = false;
        }

        add_sub_block_info.compressionModeRaw = sub_block->GetSubBlockInfo().compressionModeRaw;

        this->parent_->options_.writer->SyncAddSubBlock(add_sub_block_info);

        if (auto progress_report = this->parent_->options_.context->GetProgressReporter())
        {
            progress_report->ReportProgressCopySubBlock(
                pyramid_region.plane_coordinate,
                total_count_of_sub_blocks,
                count_of_sub_blocks_done);
        }

        ++count_of_sub_blocks_done;
    }
}

TileHelper DoPyramidize::Impl::ConstructTileHelper(const std::vector<int>& sub_blocks_of_plane) const
{
    TileHelper tile_helper;

    auto iterator = sub_blocks_of_plane.begin();
    auto end_iterator = sub_blocks_of_plane.end();
    tile_helper.InitializeFromFunctor(
        [&](IntRect& rect)->bool
        {
            if (iterator == end_iterator)
            {
                return false;
            }

            const int value = *iterator;
            ++iterator;
            SubBlockInfo sub_block_info;
            this->parent_->options_.reader->TryGetSubBlockInfo(value, &sub_block_info); // TODO(JBL): deal with error (which is a fatal one)
            rect = sub_block_info.logicalRect;
            return true;
        });
    return tile_helper;
}

std::vector<int> DoPyramidize::Impl::GetSubBlockIndicesForPlane(const PlaneEnumerator::PyramidRegion& pyramid_region) const
{
    vector<int> result;
    this->parent_->options_.reader->EnumSubset(
        &pyramid_region.plane_coordinate,
        &pyramid_region.rect,
        true,   /* onlyLayer0 */
        [&result](int index, const libCZI::SubBlockInfo&)->bool
        {
            result.push_back(index);
            return true;
        });

    return result;
}

std::shared_ptr<libCZI::IBitmapData> DoPyramidize::Impl::GeneratePyramidTile(const libCZI::IntRect& roi, const PlaneEnumerator::PyramidRegion& pyramid_region, const std::shared_ptr<libCZI::ISingleChannelTileAccessor>& tile_accessor)
{
    if (roi.w < 2 || roi.h < 2)
    {
        return nullptr;
    }

    // get the channel-index (needed for determining the background-color)
    int channel_index;
    if (!pyramid_region.plane_coordinate.TryGetPosition(DimensionIndex::C, &channel_index))
    {
        channel_index = 0;
    }

    ISingleChannelTileAccessor::Options options;
    options.Clear();
    options.useVisibilityCheckOptimization = true;
    options.backGroundColor = this->parent_->options_.context->GetBackgroundColor(channel_index);

    shared_ptr<libCZI::IBitmapData> bitmap;
    if (pyramid_region.ContainsSceneIndex())
    {
        auto plane_coordinate_without_scene = FilterOutSceneIndex(pyramid_region.plane_coordinate);
        int s_index;
        pyramid_region.plane_coordinate.TryGetPosition(DimensionIndex::S, &s_index);
        options.sceneFilter = make_shared<IndexSetOnSingleScene>(s_index);
        bitmap = tile_accessor->Get(roi, &plane_coordinate_without_scene, &options);
    }
    else
    {
        bitmap = tile_accessor->Get(roi, &pyramid_region.plane_coordinate, &options);
    }

    // OK, so now create the pyramid-tile
    auto pyramid_tile = this->parent_->options_.context->GetDecimationProcessing()->Decimate(bitmap);

    return pyramid_tile;
}

void DoPyramidize::Impl::WritePyramidOutputTile(const std::shared_ptr<libCZI::IBitmapData>& pyramid_tile, const libCZI::CDimCoordinate& coordinate, const libCZI::IntRect& roi)
{
    // first, check if the compression mode was given on the command line - if not, use the compression mode determined from the source
    libCZI::CompressionMode compression_mode = this->parent_->options_.context->GetCommandLineOptions()->GetCompressionMode();
    shared_ptr<libCZI::ICompressParameters> compression_parameters = this->parent_->options_.context->GetCommandLineOptions()->GetCompressionParameters();
    if (compression_mode == CompressionMode::Invalid)
    {
        // compression mode was not given on the command line - use the compression mode determined from the source
        const auto compression_mode_determined_from_source = this->parent_->options_.context->GetCompressionModeDeterminedFromSource();
        if (compression_mode_determined_from_source.has_value())
        {
            compression_mode = get<0>(compression_mode_determined_from_source.value());
            compression_parameters = get<1>(compression_mode_determined_from_source.value());
        }
    }

    switch (compression_mode)
    {
        case CompressionMode::UnCompressed:
        case CompressionMode::Invalid:
            this->WritePyramidOutputTileUncompressed(pyramid_tile, coordinate, roi);
            break;
        case CompressionMode::Zstd1:
            this->WritePyramidOutputTileZstd1Compressed(pyramid_tile, coordinate, roi, compression_parameters.get());
            break;
        case CompressionMode::Zstd0:
            this->WritePyramidOutputTileZstd0Compressed(pyramid_tile, coordinate, roi, compression_parameters.get());
            break;
        case CompressionMode::JpgXr:
            this->WritePyramidOutputTileJpgXrCompressed(pyramid_tile, coordinate, roi, compression_parameters.get());
            break;
        case CompressionMode::Jpg:
            throw runtime_error("Unsupported compression mode");
    }
}

void DoPyramidize::Impl::WritePyramidOutputTileJpgXrCompressed(const std::shared_ptr<libCZI::IBitmapData>& pyramid_tile, const libCZI::CDimCoordinate& coordinate, const libCZI::IntRect& roi, const ICompressParameters* compression_parameters)
{
    const ScopedBitmapLockerSP pyramid_tile_locker(pyramid_tile);
    auto compressed_bitmap = JxrLibCompress::Compress(
        pyramid_tile->GetPixelType(),
        pyramid_tile->GetWidth(),
        pyramid_tile->GetHeight(),
        pyramid_tile_locker.stride,
        pyramid_tile_locker.ptrDataRoi,
        compression_parameters);

    AddSubBlockInfoMemPtr add_sub_block_info;
    add_sub_block_info.Clear();
    add_sub_block_info.coordinate = coordinate;
    add_sub_block_info.mIndexValid = false;
    add_sub_block_info.x = roi.x;
    add_sub_block_info.y = roi.y;
    add_sub_block_info.logicalWidth = roi.w;
    add_sub_block_info.logicalHeight = roi.h;
    add_sub_block_info.physicalWidth = gsl::narrow<int>(pyramid_tile->GetWidth());
    add_sub_block_info.physicalHeight = gsl::narrow<int>(pyramid_tile->GetHeight());
    add_sub_block_info.PixelType = pyramid_tile->GetPixelType();
    add_sub_block_info.ptrData = compressed_bitmap->GetPtr();
    add_sub_block_info.dataSize = gsl::narrow<uint32_t>(compressed_bitmap->GetSizeOfData());
    add_sub_block_info.SetCompressionMode(CompressionMode::JpgXr);
    add_sub_block_info.pyramid_type = SubBlockPyramidType::MultiSubBlock;
    this->parent_->options_.writer->SyncAddSubBlock(add_sub_block_info);
}

void DoPyramidize::Impl::WritePyramidOutputTileZstd1Compressed(const std::shared_ptr<libCZI::IBitmapData>& pyramid_tile, const libCZI::CDimCoordinate& coordinate, const libCZI::IntRect& roi, const ICompressParameters* compression_parameters)
{
    const ScopedBitmapLockerSP pyramid_tile_locker(pyramid_tile);
    auto compressed_bitmap = ZstdCompress::CompressZStd1Alloc(
        pyramid_tile->GetWidth(),
        pyramid_tile->GetHeight(),
        pyramid_tile_locker.stride,
        pyramid_tile->GetPixelType(),
        pyramid_tile_locker.ptrDataRoi,
        compression_parameters);

    AddSubBlockInfoMemPtr add_sub_block_info;
    add_sub_block_info.Clear();
    add_sub_block_info.coordinate = coordinate;
    add_sub_block_info.mIndexValid = false;
    add_sub_block_info.x = roi.x;
    add_sub_block_info.y = roi.y;
    add_sub_block_info.logicalWidth = roi.w;
    add_sub_block_info.logicalHeight = roi.h;
    add_sub_block_info.physicalWidth = gsl::narrow<int>(pyramid_tile->GetWidth());
    add_sub_block_info.physicalHeight = gsl::narrow<int>(pyramid_tile->GetHeight());
    add_sub_block_info.PixelType = pyramid_tile->GetPixelType();
    add_sub_block_info.ptrData = compressed_bitmap->GetPtr();
    add_sub_block_info.dataSize = gsl::narrow<uint32_t>(compressed_bitmap->GetSizeOfData());
    add_sub_block_info.SetCompressionMode(CompressionMode::Zstd1);
    add_sub_block_info.pyramid_type = SubBlockPyramidType::MultiSubBlock;
    this->parent_->options_.writer->SyncAddSubBlock(add_sub_block_info);
}

void DoPyramidize::Impl::WritePyramidOutputTileZstd0Compressed(const std::shared_ptr<libCZI::IBitmapData>& pyramid_tile, const libCZI::CDimCoordinate& coordinate, const libCZI::IntRect& roi, const ICompressParameters* compression_parameters)
{
    const ScopedBitmapLockerSP pyramid_tile_locker(pyramid_tile);
    auto compressed_bitmap = ZstdCompress::CompressZStd0Alloc(
        pyramid_tile->GetWidth(),
        pyramid_tile->GetHeight(),
        pyramid_tile_locker.stride,
        pyramid_tile->GetPixelType(),
        pyramid_tile_locker.ptrDataRoi,
        compression_parameters);

    AddSubBlockInfoMemPtr add_sub_block_info;
    add_sub_block_info.Clear();
    add_sub_block_info.coordinate = coordinate;
    add_sub_block_info.mIndexValid = false;
    add_sub_block_info.x = roi.x;
    add_sub_block_info.y = roi.y;
    add_sub_block_info.logicalWidth = roi.w;
    add_sub_block_info.logicalHeight = roi.h;
    add_sub_block_info.physicalWidth = gsl::narrow<int>(pyramid_tile->GetWidth());
    add_sub_block_info.physicalHeight = gsl::narrow<int>(pyramid_tile->GetHeight());
    add_sub_block_info.PixelType = pyramid_tile->GetPixelType();
    add_sub_block_info.ptrData = compressed_bitmap->GetPtr();
    add_sub_block_info.dataSize = gsl::narrow<uint32_t>(compressed_bitmap->GetSizeOfData());
    add_sub_block_info.SetCompressionMode(CompressionMode::Zstd0);
    add_sub_block_info.pyramid_type = SubBlockPyramidType::MultiSubBlock;
    this->parent_->options_.writer->SyncAddSubBlock(add_sub_block_info);
}

void DoPyramidize::Impl::WritePyramidOutputTileUncompressed(const std::shared_ptr<libCZI::IBitmapData>& pyramid_tile, const libCZI::CDimCoordinate& coordinate, const libCZI::IntRect& roi)
{
    const libCZI::ScopedBitmapLockerSP pyramid_tile_locker(pyramid_tile);
    AddSubBlockInfoStridedBitmap add_sub_block_info;
    add_sub_block_info.Clear();
    add_sub_block_info.coordinate = coordinate;
    add_sub_block_info.mIndexValid = false;
    add_sub_block_info.x = roi.x;
    add_sub_block_info.y = roi.y;
    add_sub_block_info.logicalWidth = roi.w;
    add_sub_block_info.logicalHeight = roi.h;
    add_sub_block_info.physicalWidth = gsl::narrow<int>(pyramid_tile->GetWidth());
    add_sub_block_info.physicalHeight = gsl::narrow<int>(pyramid_tile->GetHeight());
    add_sub_block_info.PixelType = pyramid_tile->GetPixelType();
    add_sub_block_info.SetCompressionMode(CompressionMode::UnCompressed);
    add_sub_block_info.pyramid_type = SubBlockPyramidType::MultiSubBlock;
    add_sub_block_info.ptrBitmap = pyramid_tile_locker.ptrDataRoi;
    add_sub_block_info.strideBitmap = pyramid_tile_locker.stride;
    this->parent_->options_.writer->SyncAddSubBlock(add_sub_block_info);
}

/*static*/libCZI::CDimCoordinate DoPyramidize::Impl::FilterOutSceneIndex(const libCZI::CDimCoordinate& coordinate)
{
    libCZI::CDimCoordinate result = coordinate;
    result.Clear(libCZI::DimensionIndex::S);
    return result;
}

bool DoPyramidize::Impl::IsAbovePyramidThreshold(const libCZI::IntRect& rect) const
{
    return this->IsAbovePyramidThreshold(rect.w, rect.h);
}

bool DoPyramidize::Impl::IsAbovePyramidThreshold(std::uint32_t width, std::uint32_t height) const
{
    return
        width > this->parent_->options_.context->GetCommandLineOptions()->GetPyramidTileWidth() ||
        height > this->parent_->options_.context->GetCommandLineOptions()->GetPyramidTileHeight();
}
