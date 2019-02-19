#pragma once

#include <map>
#include <memory>

#include "parallel/Merge.h"
#include "Channel.h"

namespace Gadgetron::Grappa {

    class Image {
    public:

        struct {
            uint64_t slice;
            std::array<float, 3> position, read_dir, phase_dir, slice_dir, table_pos;
        } meta;

        hoNDArray<std::complex<float>> data;
    };

    class Weights {
    public:
        struct {
            uint64_t slice, n_combined_channels;
        } meta;

        hoNDArray<std::complex<float>> data;
    };

    class Unmixing : public Core::Parallel::Merge {
    public:
        Unmixing(const Core::Context &context, const std::unordered_map<std::string, std::string> &props);

        NODE_PROPERTY(image_series, int, "Image series number for output images", 0);

        NODE_PROPERTY(unmixing_scale, float, "", 1.0);

        void process(
                std::map<std::string, Core::InputChannel> input,
                Core::OutputChannel output
        ) override;

    private:
        static std::vector<size_t> create_output_image_dimensions(const Core::Context &context);

        const Core::Context context;
        const std::vector<size_t> image_dimensions;
    };
}
