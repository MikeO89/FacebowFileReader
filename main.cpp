#include <string>

#include "FacebowFileReader.hpp"

int main(int argc, char* argv[])
{
	const std::string mfba_file = R"(C:\Users\PatrikHuber\Projects\Facebow\2023-11-27_10-48-38-630.mfba)";

	Mimetrik::FacebowFileReader mfba_reader(mfba_file);

	const auto image_count = mfba_reader.get_image_count();

	for (int i = 0; i < image_count; ++i) {
		const auto metadata = mfba_reader.get_metadata(i);
		const auto image = mfba_reader.get_image(i);
	}

	return 0;
}
