#include <filesystem>
#include <fstream>
#include <vector>
#include <algorithm>
#include <optional>
#include <cstdint>
#include <string>

#include "nlohmann/json.hpp"
#include "opencv2/core.hpp"
#include "opencv2/imgcodecs.hpp"


struct MFBAVersion {
    std::uint8_t major;
    std::uint8_t minor;
    std::uint8_t patch;

    bool operator==(const MFBAVersion& other) const {
        return major == other.major && minor == other.minor && patch == other.patch;
    };

    bool operator!=(const MFBAVersion& other) const {
        return !(*this == other);
    };
};


/* Return whether the current system is little endian.
 *
 * From https://stackoverflow.com/a/26315033.
 *
 * @return True if the current system is little endian, false otherwise.
 */
inline bool is_little_endian()
{
    // Note: In C++20 we could just use std::endian.
    const unsigned one = 1U;
    return reinterpret_cast<const char*>(&one) + sizeof(unsigned) - 1;
};


/* Read \p num_bytes bytes from the given file starting at \p start_byte and return them.
 *
 * @param[in] mfba_file The path to the file.
 * @param[in] start_byte The byte to start reading from.
 * @param[in] num_bytes The number of bytes to read.
 * @return A vector of bytes.
 */
std::vector<std::byte> read_bytes_from_file(const std::filesystem::path& filepath, std::size_t start_byte, std::size_t num_bytes) {
    std::ifstream ifs(filepath, std::ios::binary | std::ios::ate);

    if (!ifs)
        throw std::runtime_error(filepath.string() + ": " + std::strerror(errno));

    const auto end = ifs.tellg();
    ifs.seekg(0, std::ios::beg);

    const auto size = std::size_t(end - ifs.tellg());
    // Perform some basic sanity checks on the file size and the given start/end bytes:
    if (size == 0) // avoid undefined behavior 
        throw std::runtime_error(filepath.string() + ": size == 0");
    if (start_byte > size) // >=?
        throw std::runtime_error(filepath.string() + ": start_byte > size");
    if (start_byte + num_bytes > size) // >=?
        throw std::runtime_error(filepath.string() + ": end_byte > size");

    std::vector<std::byte> buffer(num_bytes);

    ifs.seekg(start_byte, std::ios::beg);
    if (!ifs.read((char*)buffer.data(), num_bytes))
        throw std::runtime_error(filepath.string() + ": " + std::strerror(errno));

    return buffer;
};


/* Validate the MFBA file header and return the version if it is valid.
 *
 * @param[in] mfba_file The path to the MFBA file.
 * @return A pair of a bool indicating whether the header is valid and the version if it is valid.
 */
std::pair<bool, std::optional<MFBAVersion>> validate_mfba_header(const std::filesystem::path& mfba_file) {
    std::vector<std::byte> expected_signature = { std::byte('F'), std::byte('F'), std::byte('F') };
    const auto file_signature = read_bytes_from_file(mfba_file, 0, 3);
    if (file_signature != expected_signature)
        return { false, std::nullopt };

    const auto mfba_version_bytes = read_bytes_from_file(mfba_file, 3, 3);
    static_assert(sizeof(std::uint8_t) == sizeof(std::byte));
    MFBAVersion mfba_version{
        static_cast<std::uint8_t>(mfba_version_bytes[0]),
        static_cast<std::uint8_t>(mfba_version_bytes[1]),
        static_cast<std::uint8_t>(mfba_version_bytes[2])
    };
    return { true, mfba_version };
};


class FacebowFileReader {

public:
    FacebowFileReader(const std::filesystem::path& filepath) : filepath(filepath) {

        if (!std::filesystem::exists(filepath))
            throw std::runtime_error(filepath.string() + ": file does not exist");

		// Check that the file has a valid header (the first 3 bytes should be "FFF"), and read the version (the next 3 bytes):
		const auto [is_valid, mfba_version] = validate_mfba_header(filepath);
		if (!is_valid)
			throw std::runtime_error(filepath.string() + ": invalid MFBA header");
        if (mfba_version.value() != MFBAVersion{ 1, 0, 0 })
        {
            throw std::runtime_error(filepath.string() + ": MFBA version is not 1.0.0");
        }
		this->mfba_version = mfba_version.value();

        this->num_frames = read_image_count();

        // Now sweep through the file and read all header, offset and image size information:
        std::size_t frame_index = initial_frame_index;
        for (std::size_t i = 0; i < num_frames; ++i)
        {
            // Read the offset to the header ("Number of bytes from start of frame"):
            auto offset_to_header_bytes = read_bytes_from_file(filepath, frame_index, 4);
            if (is_little_endian())
                std::reverse(offset_to_header_bytes.begin(), offset_to_header_bytes.end());
            const std::uint32_t offset_to_header = *reinterpret_cast<const std::uint32_t*>(offset_to_header_bytes.data());
            
            // Read the offset to the image information ("Number of bytes from start of frame"):
            auto offset_to_image_bytes = read_bytes_from_file(filepath, frame_index + 4, 4);
            if (is_little_endian())
                std::reverse(offset_to_image_bytes.begin(), offset_to_image_bytes.end());
            const std::uint32_t offset_to_image = *reinterpret_cast<const std::uint32_t*>(offset_to_image_bytes.data());

            // Read the image size ("Size of this frame and json info (to get to next frame)"):
            auto image_size_bytes = read_bytes_from_file(filepath, frame_index + 8, 4);
            if (is_little_endian())
                std::reverse(image_size_bytes.begin(), image_size_bytes.end());
            const std::uint32_t image_size = *reinterpret_cast<const std::uint32_t*>(image_size_bytes.data());

            frame_location_info.emplace_back(FrameLocationInfo{ frame_index, offset_to_header, offset_to_image, image_size });

            frame_index += offset_to_header + offset_to_image + image_size;
        }
	};

    /* Return the number of images in the MFBA file.
     *
     * @return The number of images in the MFBA file.
     */
    std::size_t get_image_count() const {
		return num_frames;
	};


    /* Read the image metadata at index \p index from the given MFBA file and return it.
     *
     * @param[in] mfba_file The path to the MFBA file.
     * @param[in] index The index of the metadata to read.
     */
    int get_metadata(std::size_t index) {
        if (index >= num_frames)
			throw std::runtime_error("Image frame out of range, file includes " + std::to_string(num_frames) + " frames");
        
        const auto metadata_bytes = read_bytes_from_file(filepath, frame_location_info[index].frame_index + frame_location_info[index].offset_to_header, frame_location_info[index].offset_to_image);
        const auto processed_metadata = XOR(metadata_bytes);
        // Convert the sequence of bytes to ASCII. The C# code uses Encoding.ASCII.GetString. The below approach looks to be good for our use case:
        std::string metadata = "";
        for (const auto& byte : processed_metadata)
        {
            metadata += static_cast<char>(byte);
        };
        // Now convert the string to a JSON object:
        nlohmann::json json_metadata = nlohmann::json::parse(metadata);

        /* We probably want the following? Some of them are duplicated, there is "metadataSource": "CameraCharacteristics" and "metadataSource": "CaptureResult". We may want the latter, if the values are not identical?
            "key": "android.lens.distortion",
            "value": "[0.0883828, -0.16732782, 0.1392884, 0.0, 0.0]"

            "key": "android.lens.info.availableFocalLengths",
            "value": "[5.43]"

        */

        // json_metadata[2] contains "metadataSource: CaptureResult", which then has a list of key-value pairs in "contents". We'll copy it into a map so we can access its values more easily:
        std::map<std::string, std::string> capture_result_metadata;
        for (const auto& e : json_metadata[2]["contents"])
        {
            capture_result_metadata.emplace(e["key"].template get<std::string>(), e["value"].template get<std::string>());
        }

        // Todo: Find out which values we want, and return them.
        return 0;
    };

    /* Read the image at index \p index from the given MFBA file and return it.
     *
     * @param[in] mfba_file The path to the MFBA file.
     * @param[in] index The index of the image to read.
     */
    cv::Mat get_image(std::size_t index) {
        if (index >= num_frames)
            throw std::runtime_error("Image frame out of range, file includes " + std::to_string(num_frames) + " frames");

        const auto imagedata_bytes = read_bytes_from_file(filepath, frame_location_info[index].frame_index + frame_location_info[index].offset_to_header + frame_location_info[index].offset_to_image, frame_location_info[index].image_size);
        const auto processed_imagedata = XOR(imagedata_bytes);

        cv::Mat image(image_height, image_width, CV_8UC3);
        std::size_t i = 0;
        for (int row = 0; row < image_height; ++row)
		{
			for (int col = 0; col < image_width; ++col)
			{
                // The data is stored in BGR order - since OpenCV uses BGR by default, we don't need to swap the order:
				image.at<cv::Vec3b>(row, col)[0] = static_cast<std::uint8_t>(processed_imagedata[i++]);
				image.at<cv::Vec3b>(row, col)[1] = static_cast<std::uint8_t>(processed_imagedata[i++]);
				image.at<cv::Vec3b>(row, col)[2] = static_cast<std::uint8_t>(processed_imagedata[i++]);
			}
		}

        return image;
    };

    /* Stores the header information for a frame in the MFBA file.
     */
    struct FrameLocationInfo {
        std::size_t frame_index;
        std::uint32_t offset_to_header;
        std::uint32_t offset_to_image;
        std::uint32_t image_size;
    };

private:
    std::filesystem::path filepath;
    const int image_width = 1080;
    const int image_height = 1920;
    std::size_t initial_frame_index = 8; // 3 signature bytes + 3 version bytes + 2 bytes for num_frames
    std::size_t num_frames = 0;
    MFBAVersion mfba_version;
    std::vector<FrameLocationInfo> frame_location_info;


    /* Return the number of images in the given MFBA file.
     *
     * @param[in] mfba_file The path to the MFBA file.
     * @return The number of images in the MFBA file.
     */
    std::size_t read_image_count() {
        // Note: 2 bytes are reserved for this in the MFBA file header.
        auto number_of_frames_as_bytes = read_bytes_from_file(filepath, 6, 2);
        if (is_little_endian())
            std::reverse(number_of_frames_as_bytes.begin(), number_of_frames_as_bytes.end());

        const std::uint16_t number_of_frames = *reinterpret_cast<const std::uint16_t*>(number_of_frames_as_bytes.data());
        return static_cast<std::size_t>(number_of_frames);
    };

    std::vector<std::byte> XOR(const std::vector<std::byte>& input) { 
        std::vector<std::byte> output(input.size());
		for (std::size_t i = 0; i < input.size(); ++i)
			output[i] = input[i] ^ std::byte(0xFF);
		return output;
    }
};
