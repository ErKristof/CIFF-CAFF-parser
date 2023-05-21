#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <iostream>
#include <fstream>
#include <string>
#include <optional>
#include <vector>
#include <filesystem>
#include "stb_image_write.h"

struct CAFFBlockType {
public:
	static const uint8_t header = '\x01';
	static const uint8_t credits = '\x02';
	static const uint8_t animation = '\x03';
};

struct CAFFBlockHeader {
	uint8_t id;
	size_t length;
};

//Method to check if the file still has enough bytes to read
bool canReadBytes(std::ifstream& file, std::streampos currentPos, std::streamsize numBytes) {
	file.seekg(currentPos + numBytes);
	if (file && file.tellg() != -1) {
		file.seekg(currentPos);
		return true;
	}
	file.seekg(currentPos);
	return false;
}
//Converter method from an 8 byte char array to a size_t
size_t convertToNumber64(char chars[8]) {
	size_t number = 0;
	for (int i = 0; i < 8; i++) {
		number = (number << 8) | static_cast<uint8_t>(chars[7 - i]);
	}
	return number;
}
//Converter method from a 2 byte char array to a uint16_t
uint16_t convertToNumber16(char chars[2]) {
	uint16_t number = 0;
	for (int i = 0; i < 2; i++) {
		number = (number << 8) | static_cast<uint8_t>(chars[1 - i]);
	}
	return number;
}

//Gets the file stream 
std::optional<CAFFBlockHeader> readCAFFBlockHeader(std::ifstream& file) {
	//Check if the filestream is still good
	if (!file.good()) {
		std::cerr << "Failed to read file!" << std::endl;
		return std::nullopt;
	}
	//Check if there are 9 bytes to read in the file
	if (!canReadBytes(file, file.tellg(), 9)) {
		std::cerr << "Not enough bytes left in the file!" << std::endl;
		return std::nullopt;
	}
	//Block ID byte
	char id;
	//Block length integer
	char length[8];

	//Read the ID and length fields
	if (!file.get(id)) {
		std::cerr << "Failed to read file!" << std::endl;
		return std::nullopt;
	}
	if (!file.read(length, sizeof(length))) {
		std::cerr << "Failed to read file!" << std::endl;
		return std::nullopt;
	}

	//Convert the char bytes to integer types
	uint8_t iid = static_cast<uint8_t>(id);
	size_t ilength = convertToNumber64(length);

	//Make the block header struct
	CAFFBlockHeader header = { iid, ilength };

	//Check if it's a header block and the length is correctly 20 bytes (magic(4) + header_size(8) + num_anim(8))
	if (header.id == CAFFBlockType::header && header.length == 20) {
		return header;
	}
	//Check if it's a credits block and the length is at least 14 bytes (date(6) + creator_len(8))
	if (header.id == CAFFBlockType::credits && header.length >= 14) {
		return header;
	}
	//Check if it's an animation block and the length is at least 42 bytes (duration(8) + CIFF headers (36))
	if (header.id == CAFFBlockType::animation && header.length >= 42) {
		return header;
	}
	//If ID is not those types return nullopt
	std::cerr << "Id or length of CAFF block is not correct! " << std::endl << "Header Id: " << header.id << std::endl << "Header length: " << header.length << std::endl;
	return std::nullopt;
}

//Read and check the CAFF Header block data
bool readCAFFHeaderBlock(std::ifstream& file) {
	//Check if the filestream is still good
	if (!file.good()) {
		std::cerr << "Failed to read file!" << std::endl;
		return false;
	}
	//Check if we have enough space in the file to read
	if (!canReadBytes(file, file.tellg(), 20)) {
		std::cerr << "Not enough bytes left in the file!" << std::endl;
		return false;
	}
	//Header magic characters
	char magic[4];
	//Header size integer
	char header_size[8];
	//Number of animation blocks integer
	char num_anim[8];

	//Read the header fields
	if (!file.read(magic, sizeof(magic))) {
		std::cerr << "Failed to read file!" << std::endl;
		return false;
	}
	if (!file.read(header_size, sizeof(header_size))) {
		std::cerr << "Failed to read file!" << std::endl;
		return false;
	}
	if (!file.read(num_anim, sizeof(num_anim))) {
		std::cerr << "Failed to read file!" << std::endl;
		return false;
	}
	//Convert char bytes
	size_t iheader_size = convertToNumber64(header_size);
	size_t inum_anim = convertToNumber64(num_anim);
	std::string smagic(magic, sizeof(magic));

	//Check if the magic characters are "CAFF"
	if (smagic != "CAFF") {
		std::cerr << "Magic is not CAFF" << std::endl << "Magic: " << smagic << std::endl;
		return false;
	}
	//Check if the header size is equal to 20 (magic(4) + header_size(8) + num_anim(8))
	if (iheader_size != 20) {
		std::cerr << "Header size is not correct" << std::endl << "Header size: " << iheader_size << std::endl;
		return false;
	}
	//Check if there are CIFFs to parse
	if (inum_anim < 1) {
		std::cerr << "No CIFF image to convert!" << std::endl;
		return false;
	}
	return true;
}

//Read and check the CAFF Creadits block data
bool readCAFFCreditsBlock(std::ifstream& file, size_t credits_length) {
	//Check if the filestream is still good
	if (!file.good()) {
		std::cerr << "Failed to read file!" << std::endl;
		return false;
	}
	//Check if the file has enough data to read for the credits
	if (!canReadBytes(file, file.tellg(), credits_length)) {
		std::cerr << "Not enough bytes left in the file!" << std::endl;
		return false;
	}
	//Creation date 
	char year[2];
	char month;
	char day;
	char hour;
	char minute;
	//Creator length integer
	char creator_length[8];

	//Read date and creator length
	if (!file.read(year, sizeof(year))) {
		std::cerr << "Failed to read file!" << std::endl;
		return false;
	}
	if (!file.get(month)) {
		std::cerr << "Failed to read file!" << std::endl;
		return false;
	}
	if (!file.get(day)) {
		std::cerr << "Failed to read file!" << std::endl;
		return false;
	}
	if (!file.get(hour)) {
		std::cerr << "Failed to read file!" << std::endl;
		return false;
	}
	if (!file.get(minute)) {
		std::cerr << "Failed to read file!" << std::endl;
		return false;
	}
	if (!file.read(creator_length, sizeof(creator_length))) {
		std::cerr << "Failed to read file!" << std::endl;
		return false;
	}

	//Convert the date and length fields to integers
	uint16_t iyear = convertToNumber16(year);
	int imonth = static_cast<int>(month);
	int iday = static_cast<int>(day);
	int ihour = static_cast<int>(hour);
	int iminute = static_cast<int>(minute);
	size_t icreator_length = convertToNumber64(creator_length);

	//Check if the date format is correct
	if (iyear < 0 || iyear > 9999) {
		std::cerr << "Year is not correct!" << std::endl << "Year: " << iyear << std::endl;
		return false;
	}
	if (imonth < 1 || imonth > 12) {
		std::cerr << "Month is not correct!" << std::endl << "Month: " << imonth << std::endl;
		return false;
	}
	if (iday < 1 || iday > 31) {
		std::cerr << "Day is not correct!" << std::endl << "Day: " << iday << std::endl;
		return false;
	}
	if (ihour < 0 || ihour > 24) {
		std::cerr << "Hour is not correct!" << std::endl << "Hour: " << ihour << std::endl;
		return false;
	}
	if (iminute < 0 || iminute > 60) {
		std::cerr << "Minute is not correct!" << std::endl << "Minute: " << iminute << std::endl;
		return false;
	}
	//Check if the creator matches up with the CAFF block length
	if (icreator_length != credits_length - 14) {
		std::cerr << "Creator length mismatch!" << std::endl << "Creator length is: " << icreator_length << " when it should be: " << credits_length -14 << std::endl;
		return false;
	}
	//If there is no creator return and parsing can continue
	if (icreator_length != 0) {
		//Create char array for the creator string
		char* creator = new char[icreator_length];

		//Read in the creator string
		if (!file.read(creator, std::streamsize(icreator_length))) {
			std::cerr << "Failed to read file!" << std::endl;
			delete[] creator;
			return false;
		}
		//Convert to string
		std::string screator(creator, icreator_length);
		delete[] creator;

		//Print the creator
		std::cout << "CAFF Creator: " << screator << std::endl;
	}
	//Print creation time
	std::cout << "Creation date: " << iyear << "." << imonth << "." << iday << ". " << ihour << ":" << iminute << std::endl;
	return true;
}

//Read and verify the CIFF file and make the JPEG after
bool readCIFFFile(std::ifstream& file, std::string fileName) {
	//Check if the filestream is still good
	if (!file.good()) {
		std::cerr << "Failed to read file!" << std::endl;
		return false;
	}
	//Check if the file has enough data to read the CIFF headers
	if (!canReadBytes(file, file.tellg(), 36)) {
		std::cerr << "Not enough bytes left in the file!" << std::endl;
		return false;
	}
	//CIFF magic characters
	char magic[4];
	//CIFF header size
	char header_size[8];
	//CIFF content size
	char content_size[8];
	//CIFF image width
	char width[8];
	//CIFF image height
	char height[8];

	//Read in the header fields
	if (!file.read(magic, sizeof(magic))) {
		std::cerr << "Failed to read file!" << std::endl;
		return false;
	}
	if (!file.read(header_size, sizeof(header_size))) {
		std::cerr << "Failed to read file!" << std::endl;
		return false;
	}
	if (!file.read(content_size, sizeof(content_size))) {
		std::cerr << "Failed to read file!" << std::endl;
		return false;
	}
	if (!file.read(width, sizeof(width))) {
		std::cerr << "Failed to read file!" << std::endl;
		return false;
	}
	if (!file.read(height, sizeof(height))) {
		std::cerr << "Failed to read file!" << std::endl;
		return false;
	}

	//Convert the fields to numbers and string
	std::string smagic(magic, sizeof(magic));
	size_t iheader_size = convertToNumber64(header_size);
	size_t icontent_size = convertToNumber64(content_size);
	size_t iwidth = convertToNumber64(width);
	size_t iheight = convertToNumber64(height);

	//Check if magic characters are CIFF
	if (smagic != "CIFF") {
		std::cerr << "Magic is not CIFF" << std::endl << "Magic: " << smagic << std::endl;
		return false;
	}

	//Check if the header size is at least 36 characters
	if (iheader_size <= 36) {
		std::cerr << "Header size is incorrect" << std::endl << "Header size: " << iheader_size << std::endl;
		return false;
	}

	//Check if the Content size is width * height * 3
	if (icontent_size != iwidth * iheight * 3) {
		std::cerr << "Content size is incorrect!" << "Content size: " << icontent_size << " != " << iwidth << " * " << iheight << " * " << "3" << std::endl;
		return false;
	}

	//Check if there are pixels to convert
	if (icontent_size == 0) {
		std::cerr << "No pixels to make JPEG!" << std::endl;
		return false;
	}
	
	//Calculate the remaining size of the header
	size_t remaining_header_size = iheader_size - 36;

	//Check if the caption and tags have at least the ending characters
	if (remaining_header_size < 2) {
		std::cerr << "Header size is incorrect!" << std::endl;
		return false;
	}
	//Check if the file has enough data to read the CIFF headers
	if (!canReadBytes(file, file.tellg(), remaining_header_size)) {
		std::cerr << "Not enough bytes left in file!" << std::endl;
		return false;
	}
	//Read the string until we get a '\n' or run out of header space
	char ch;
	size_t counter = 0;
	std::string caption;
	bool reading = true;
	while (reading && file.get(ch)) {
		if (counter >= remaining_header_size) {
			std::cerr << "No closing '\\n' in caption!" << std::endl;
			return false;
		}
		if (ch == '\n') {
			reading = false;
		}
		caption += ch;
		counter++;
	}
	//Subtract the length of the caption from the remaining header size
	remaining_header_size -= caption.length();

	//The rest of the space is for the tags
	char* tags = new char[remaining_header_size];

	//Read in the remaining tags
	if (!file.read(tags, std::streamsize(remaining_header_size))) {
		std::cerr << "Failed to read file!" << std::endl;
		delete[] tags;
		return false;
	}

	//Check if tags have '\n' in them
	for (size_t i = 0; i < remaining_header_size; i++) {
		if (tags[i] == '\n') {
			std::cerr << "Tags contain '\\n' character!" << std::endl;
			return false;
		}
	}
	std::vector<std::string> vtags;

	//Go through the tags array and divide them by '\0'
	std::string currentTag;
	for (size_t i = 0; i < remaining_header_size; i++) {
		if (tags[i] != '\0') {
			currentTag += tags[i];
		}
		else {
			currentTag += tags[i];
			vtags.push_back(currentTag);
			currentTag.clear();
		}
	}
	delete[] tags;

	//Make the JPEG file from the content of the CIFF
	//Check if the file has enough space for the files
	if (!canReadBytes(file, file.tellg(), icontent_size)) {
		std::cerr << "Not enough bytes left in file!" << std::endl;
		return false;
	}

	//Make the buffer for the JPEG content
	char* buffer = new char[icontent_size];
	if (!file.read(buffer, icontent_size)) {
		std::cerr << "Failed to read file!" << std::endl;
		return false;
	}
	//Make the file name
	std::string name = fileName + ".jpg";

	//Make the file.
	int result = stbi_write_jpg(name.c_str(), (int)iwidth, (int)iheight, 3, buffer, 50);

	//If the result is 0 it was not successful
	if (result == 0) {
		std::cerr << "Failed to make JPEG file!" << std::endl;
		return false;
	}
	//Print CIFF data
	std::cout << "CIFF size: " << iwidth << " x " << iheight << std::endl;
	std::cout << "Caption: " << caption << std::endl;
	std::cout << "Tags: ";
	for (size_t i = 0; i < vtags.size(); i++) {
		std::cout << vtags[i] << " ";
	}
	std::cout << std::endl;
	return true;
}

//Read in and verify the CAFF animation block.
//If successfully verified call the CIFF parser and make the JPEG file
bool readCAFFAnimationBlock(std::ifstream& file, std::string fileName, size_t animation_length) {
	//Check if the filestream is still good
	if (!file.good()) {
		std::cerr << "Failed to read file!" << std::endl;
		return false;
	}
	//Check if the file has enough data to read the block
	if (!canReadBytes(file, file.tellg(), animation_length)) {
		std::cerr << "Failed to read file!" << std::endl;
		return false;
	}

	//Animation duration
	char duration[8];

	//Read in the duration
	if (!file.read(duration, sizeof(duration))) {
		std::cerr << "Failed to read file!" << std::endl;
		return false;
	}
	//Convert the duration
	//size_t iduration = convertToNumber64(duration);
	
	//Read and verify the CIFF file and make the JPEG
	if (!readCIFFFile(file, fileName)) {
		std::cerr << "Failed to parse CIFF file!" << std::endl;
		return false;
	}

	return true;
}

//Read the CAFF files
//Returns with true if successful, otherwise false
bool readCAFFFile(std::ifstream& file, std::string fileName) {
	//Start reading CAFF file
	//Read until we get one animation block or something goes wrong
	bool finished = false;
	while (!finished) {

		//Read the current CAFF block header
		std::optional<CAFFBlockHeader> currentBlockOpt = readCAFFBlockHeader(file);

		//If something went wrong return with false
		if (!currentBlockOpt.has_value()) {
			std::cerr << "Failed to parse CAFF Block!" << std::endl;
			return false;
		}
		//Get the value of the current block and based on its type call the correct function
		CAFFBlockHeader currentBlock = currentBlockOpt.value();

		switch (currentBlock.id) {
		//If the block is a header block read it and verify it
		//If successfully verified continue reading else return with false
		case CAFFBlockType::header:
			if (!readCAFFHeaderBlock(file)) {
				std::cerr << "Failed to parse CAFF Header Block!" << std::endl;
				return false;
			}
			break;
		//If the block is a credits block read it and verify it
		//If successfully verified continue reading else return with false
		case CAFFBlockType::credits:
			if (!readCAFFCreditsBlock(file, currentBlock.length)) {
				std::cerr << "Failed to parse CAFF Credits Block!" << std::endl;
				return false;
			}
			break;
		//If the block is an animation block read it, verify it and make the JPEG file
		//If successfully verified and the file is made return with true else return with false
		case CAFFBlockType::animation:
			if (!readCAFFAnimationBlock(file, fileName, currentBlock.length)) {
				std::cerr << "Failed to parse CAFF Animation Block!" << std::endl;
				return false;
			}
			finished = true;
			break;
		}

	}
	return true;
}



int main(int argc, char* argv[])
{
	//Check to see if it was called with two arguments
	if (argc != 3) {
		std::cerr << "Invalid number of arguments!" << std::endl;
		return -1;
	}

	//Process the arguments
	std::string command = argv[1];
	std::string filePath = argv[2];
	std::string fileName;

	//Check the lengths of the arguments
	if (command.length() != 5 || filePath.length() > 260 || filePath.length() < 6) {
		std::cerr << "Invalid parameters!" << std::endl;
		return -1;
	}

	//Check if the file exists and is a file
	std::filesystem::path path(filePath);
	if (!std::filesystem::exists(path) || !std::filesystem::is_regular_file(path)) {
		std::cerr << "Incorrect file path!" << std::endl;
		return -1;
	}
	//Set the file name
	fileName = path.filename().string();

	//Check for CIFF or CAFF file
	if (command == "-caff" && fileName.substr(fileName.length() - 5) == ".caff") {
		//Get the name of the file
		fileName = fileName.substr(0, fileName.length() - 5);
		//Try to open the file
		std::ifstream file(filePath, std::ios::binary);
		if (!file) {
			std::cerr << "Failed to open file!" << std::endl;
			return -1;
		}
		//Read the CAFF file and make the JPEG
		if (!readCAFFFile(file, fileName)) {
			file.close();
			return -1;
		}
		//Close the file
		file.close();
	}
	else if (command == "-ciff" && fileName.substr(fileName.length() - 5) == ".ciff") {
		//Get the name of the file
		fileName = fileName.substr(0, fileName.length() - 5);
		//Try to open the file
		std::ifstream file(filePath, std::ios::binary);
		if (!file) {
			std::cerr << "Failed to open file!" << std::endl;
			return -1;
		}
		//Read the CIFF file and make the JPEG
		if (!readCIFFFile(file, fileName)) {
			std::cerr << "Failed to parse CIFF file!" << std::endl;
			file.close();
			return -1;
		}
		//Close the file
		file.close();
	}
	else {
		std::cerr << "Invalid parameters!" << std::endl;
		return -1;
	}
	return 0;
}
