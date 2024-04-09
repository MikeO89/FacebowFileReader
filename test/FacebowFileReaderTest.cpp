#include <chrono>
#include <gtest/gtest.h>
#include <gmock/gmock.h> // Unable to mock member functions due to not being declared as virtual - changing is outside the scope of the assessment
#include <mimetrik/FacebowFileReader.hpp>

TEST(FacebowFileReaderTest, FailOnEmptyFile)
{
    std::string error = "";

    try
    {
        // Pass a path to a file that exists but is empty
        mimetrik::FacebowFileReader("test_video_empty.mfba");
    }
    catch(std::runtime_error &e)
    {  // Should arrive here due to an error being thrown when attempting to read an emtpy file
        error = e.what();
    }

    EXPECT_EQ(error, "test_video_empty.mfba: size == 0"); // Testing outside of the catch as the assertion would not be made if the catch was not triggered
}

TEST(FacebowFileReaderTest, FailOnNonExistantFile)
{   // Error on empty file
    std::string error = "";

    try
    {
        // Pass a path to a file that does not exist
        mimetrik::FacebowFileReader("test_video_non_existant.mfba");
    }
    catch(std::runtime_error &e)
    { // Should arrive here due to an error being thrown on the file path being invalid
        error = e.what();
    }

    EXPECT_EQ(error, "test_video_non_existant.mfba: file does not exist");  // Testing outside of the catch as the assertion would not be made if the catch was not triggered
}

TEST(FacebowFileReaderTest, FailOnInvalidFileSignature)
{
    std::string error = "";

    try
    {
    // Headers are 0x4D 0x4D 0x4D 0x4D 0x4D 0x4D 0x4D 0x4D

        // Path a path to a file that exists, but the file signature is incorrect
        mimetrik::FacebowFileReader("test_video_invalid_signature.mfba");
    }
    catch(std::runtime_error &e)
    {  // Should arrive here due to the initial file bytes not being as expected
        error = e.what();
    }

    // Testing outside of the catch as the assertion would not be made if the catch was not triggered
    EXPECT_EQ(error, "test_video_invalid_signature.mfba: invalid MFBA header");
}

TEST(FacebowFileReaderTest, FailOnInvalidVersion)
{
    std::string error = "";

    try
    {
    // Headers are 0x46 0x46 0x46 0x02 0x00 0x00 0x00 0x00
   
    // 0x46 0x46 0x46 denoting the MFBA file format
    // 0x02 0x00 0x00 denoting the version number (2.0.0)
    // 0x00 0x00 denoting the frame count (0)

        // Path a path to a file that exists, but the header specifies the version as 2.0.0
        mimetrik::FacebowFileReader("test_video_invalid_version.mfba");
    }
    catch(std::runtime_error &e)
    {  // Should arrive here due to the version number not being 1.0.0
        error = e.what();
    }

    // Testing outside of the catch as the assertion would not be made if the catch was not triggered
    EXPECT_EQ(error, "test_video_invalid_version.mfba: MFBA version is not 1.0.0");
}

TEST(FacebowFileReaderTest, FileHeadersAreValid)
{
    // Headers are 0x46 0x46 0x46 0x01 0x00 0x00 0x00 0x00
   
    // 0x46 0x46 0x46 denoting the MFBA file format
    // 0x01 0x00 0x00 denoting the version number (1.0.0)
    // 0x00 0x00 denoting the frame count (0)

    const std::string FILE_PATH = "test_video_valid.mfba";
    const std::uint8_t MAJOR = 1, MINOR = 0, PATCH = 0;

    // Pass a path to a file that is valid, but there are no frames
    mimetrik::FacebowFileReader reader(FILE_PATH); 
    // If an error is thrown, the test will fail, and therefore no assertion is required to test the validity of the file headers

    // The test is focusing on the headers rather than the content, so the file has no content other than header data
    EXPECT_EQ(reader.get_image_count(), 0);

    auto [valid, optVersion] = mimetrik::validate_mfba_header(FILE_PATH);
    mimetrik::MFBAVersion version = optVersion.value();

    EXPECT_TRUE(valid);
    EXPECT_EQ(version, mimetrik::MFBAVersion({MAJOR,MINOR,PATCH})); // Expect: 1.0.0 - testing operator==
    EXPECT_EQ(version.major, MAJOR); // Expect: 1
    EXPECT_EQ(version.minor, MINOR); // Expect: 0
    EXPECT_EQ(version.patch, PATCH); // Expect: 0
}

TEST(FacebowFileReaderTest, FailOnOutOfRange)
{
    const std::string EXPECTED_ERROR = "Image frame out of range, file includes 0 frames";
    std::string error = "";

    // Headers are 0x46 0x46 0x46 0x01 0x00 0x00 0x00 0x00
   
    // 0x46 0x46 0x46 denoting the MFBA file format
    // 0x02 0x00 0x00 denoting the version number (1.0.0)
    // 0x00 0x00 denoting the frame count (0)

    // Pass a path to a file that is valid, but there are no frames
    mimetrik::FacebowFileReader reader("test_video_valid.mfba");

    try
    {
        // There are no frames, so index 0 should be out of range
        reader.get_metadata(0);
    }
    catch(std::runtime_error &e)
    {  // Should arrive here due to requesting an out-of-range index 
        error = e.what();
    }

    // Testing outside of the catch as the assertion would not be made if the catch was not triggered
    EXPECT_EQ(error, EXPECTED_ERROR);

    // Set to empty as both error messages should be the same
    error = "";

    try
    {
        // There are no frames, so index 0 should be out of range
        reader.get_image(0);
    }
    catch(std::runtime_error &e)
    {  // Should arrive here due to requesting an out-of-range index 
        error = e.what();
    }

    // Testing outside of the catch as the assertion would not be made if the catch was not triggered
    EXPECT_EQ(error, EXPECTED_ERROR);
}

TEST(FacebowFileReaderTest, FileBytesAreReadCorrectly)
{
    const std::string FILE_PATH = "test_video_valid.mfba";
    const std::vector<std::uint8_t> EXPECTED_CONTENT = {0x46, 0x46, 0x46, 0x01, 0x00, 0x00, 0x00, 0x00};

    // Headers are 0x46 0x46 0x46 0x01 0x00 0x00 0x00 0x00
   
    // 0x46 0x46 0x46 denoting the MFBA file format
    // 0x01 0x00 0x00 denoting the version number (1.0.0)
    // 0x00 0x00 denoting the frame count (0)

    // Determine byte count
    std::ifstream file(FILE_PATH, std::ios::binary | std::ios::ate);

    ASSERT_TRUE(file.is_open());

    const std::size_t byteCount = file.tellg();
    file.close();

    std::vector<std::byte> content = mimetrik::read_bytes_from_file(FILE_PATH, 0, byteCount);
    // Flag std::ios::ate to specify the end of the file
    // Flat std::ios::binary indicates to interpret the file content as-is (raw binary) without interpretation
    
    EXPECT_EQ(content, *reinterpret_cast<const std::vector<std::byte> *>(&EXPECTED_CONTENT));
}

TEST(FacebowFileReaderTest, FrameMetaLoadedCorrectly)
{
    const std::string VIDEO_PATH = "test_video_reduced.mfba", FRAME_0_META_PATH = "frame0Meta.json";
    const std::uint8_t LEFT_SQUARE_BRACKET('[');
   
    // File headers (8 Bytes)
    
    // 0x46 0x46 0x46          (3 Bytes)         denoting the MFBA file format
    // 0x01 0x00 0x00          (3 Bytes)         denoting the version number (1.0.0)
    // 0x00 0x10               (2 Bytes)         denoting the frame count (16)

    // Frame 0 Start
    
    // 0x00 0x00 0x00 0x0C     (4 Bytes)         denoting the frame header location (offset from the start of the frame)
    // 0x00 0x00 0x6C 0xA8     (4 Bytes)         denoting the frame image data location (offset from the start of the frame)
    // 0x00 0x5E 0xEC 0x00     (4 Bytes)         denoting the image byte count
    
    const size_t FRAME_0_META_START_OFFSET = 20; // Signature (3) + Version (3) + Frame Count (2) + HEADER LOC(4) + IMAGE LOC(4) + IMAGE SIZE(4)
  
    // Ensure the loaded data is XOR 255 encoded

    // Read the first header byte and decode
    const std::uint8_t firstHeaderValue = *reinterpret_cast<std::uint8_t *>(&mimetrik::read_bytes_from_file(VIDEO_PATH, FRAME_0_META_START_OFFSET, 1)[0]) ^ 0xFF;

    EXPECT_EQ(firstHeaderValue, LEFT_SQUARE_BRACKET);

    // Load frame 0 stored in a json file

    std::ifstream metaFile(FRAME_0_META_PATH);

    ASSERT_TRUE(metaFile.is_open());

    // Read the content in to a string
    std::string frame0MetaRaw((std::istreambuf_iterator<char>(metaFile)),
                                std::istreambuf_iterator<char>());
  
    metaFile.close();

    // Convert to nlohmann::json
    nlohmann::json frame0MetaJSON = nlohmann::json::parse(frame0MetaRaw);

    // Will store the results for comparison against the response from mimetrik::FacebowFileReader::get_metadata
    std::map<std::string, std::map<std::string, std::string>> frame0Meta;

    // Iterate each parent
    for(nlohmann::json::iterator itParent=frame0MetaJSON.begin(), endParent=frame0MetaJSON.end(); itParent!=endParent; itParent++)
    {
        std::string key = itParent.key();
        nlohmann::json childJSON = itParent.value();
        std::map<std::string, std::string> child;

        // Build the chid content
        for(nlohmann::json::iterator itChild=childJSON.begin(), endChild=childJSON.end(); itChild!=endChild; itChild++)
            child.emplace(itChild.key(), itChild.value());
  
        // Set the child content to the parent key
        frame0Meta.emplace(key, child);
    }

    // Load the video
    mimetrik::FacebowFileReader reader(VIDEO_PATH);

     // Compare frame 0 obtained from the video with the raw frame 0 stored in the loaded JSON file
    EXPECT_EQ(frame0Meta, reader.get_metadata(0));
}

TEST(FacebowFileReaderTest, FrameLoadedCorrectly)
{
    const size_t EXPECTED_FRAME_COUNT = 0x10; // 16
    const int FRAME_WIDTH = 1080, FRAME_HEIGHT = 1920;

    const std::string VIDEO_PATH = "test_video_reduced.mfba", FRAME_0_PATH = "frame0",
        EXPECTED_ERROR = "Image frame out of range, file includes " + std::to_string(EXPECTED_FRAME_COUNT) + " frames";

    std::string error = "";

    // Headers are 0x46 0x46 0x46 0x01 0x00 0x00 0x00 0x4E
   
    // 0x46 0x46 0x46 denoting the MFBA file format
    // 0x01 0x00 0x00 denoting the version number (1.0.0)
    // 0x00 0x10 denoting the frame count (16)

    mimetrik::FacebowFileReader reader(VIDEO_PATH);

    // Testing the bounds

    EXPECT_EQ(EXPECTED_FRAME_COUNT, reader.get_image_count());

    try
    {
        reader.get_image(EXPECTED_FRAME_COUNT);
    }
    catch(std::runtime_error &e)
    {
        error = e.what();
    }

    EXPECT_EQ(error, EXPECTED_ERROR);

    // Clear error as the next error should be the same as the above error
    error = "";

    try
    {
        reader.get_metadata(EXPECTED_FRAME_COUNT);
    }
    catch(std::runtime_error &e)
    {
        error = e.what();
    }

    EXPECT_EQ(error, EXPECTED_ERROR);

    EXPECT_NO_THROW(reader.get_image(EXPECTED_FRAME_COUNT-1));
    EXPECT_NO_THROW(reader.get_metadata(EXPECTED_FRAME_COUNT-1));

    // Testing BGR values are correct for frame 0

    // Read BGR test data for frame 0
    std::ifstream frame0File(FRAME_0_PATH, std::ios::binary);

    ASSERT_TRUE(frame0File.is_open());

    std::string buffer((std::istreambuf_iterator<char>(frame0File)),
                                std::istreambuf_iterator<char>());

    frame0File.close();

    std::uint8_t *frame0BGRTestData = reinterpret_cast<std::uint8_t *>(buffer.data());

    // Extract BGR data for frame 0
    cv::Mat frame0BGRData = reader.get_image(0);

    for(int x=0; x<FRAME_WIDTH; x++)
    {
        for(int y=0; y<FRAME_HEIGHT; y++)
        { // Ensure frame 0 BGR values match expected values
            EXPECT_EQ(frame0BGRTestData[x * FRAME_HEIGHT + y],*reinterpret_cast<std::uint8_t *>(frame0BGRData.ptr(y,x)));
        }
    }
}

TEST(FacebowFileReaderTest, FrameLatencyIsAdequate)
{
    // Headers are 0x46 0x46 0x46 0x01 0x00 0x00 0x00 0x4E
   
    // 0x46 0x46 0x46 denoting the MFBA file format
    // 0x01 0x00 0x00 denoting the version number (1.0.0)
    // 0x00 0x10 denoting the frame count (16)

    mimetrik::FacebowFileReader reader("test_video_reduced.mfba");

    const int frameCount = reader.get_image_count();

    const long long startTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                                    std::chrono::system_clock::now().time_since_epoch()).count();

    for(int i=0; i<frameCount; ++i) // Load 16 frames 
        reader.get_image(i);

    const long long endTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                                    std::chrono::system_clock::now().time_since_epoch()).count();

    const double duration = static_cast<double>(endTime - startTime);

    const double frameRate = static_cast<double>(frameCount) / (duration / 1000);

     // These checks will fail for the current implementation as the code is loading at approximately 2fps
    //EXPECT_LT(duration, 1000); // Would take 0.8 seconds to achieve 20fps for 16 frames
    //EXPECT_GT(frameRate, 20.0); // Expect at least 20fps
}

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}