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
	unsigned char id = 0;
	//Block length integer
	size_t length = 0;

	//Read the ID and length fields
	if (!file.read(reinterpret_cast<char*>(&id), sizeof(id))) {
		std::cerr << "Failed to read file!" << std::endl;
		return std::nullopt;
	}
	if (!file.read(reinterpret_cast<char*>(&length), sizeof(length))) {
		std::cerr << "Failed to read file!" << std::endl;
		return std::nullopt;
	}

	//Make the block header struct
	CAFFBlockHeader header = { id, length };

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
	size_t header_size = 0;
	//Number of animation blocks integer
	size_t num_anim = 0;

	//Read the header fields
	if (!file.read(magic, sizeof(magic))) {
		std::cerr << "Failed to read file!" << std::endl;
		return false;
	}
	if (!file.read(reinterpret_cast<char*>(&header_size), sizeof(header_size))) {
		std::cerr << "Failed to read file!" << std::endl;
		return false;
	}
	if (!file.read(reinterpret_cast<char*>(&num_anim), sizeof(num_anim))) {
		std::cerr << "Failed to read file!" << std::endl;
		return false;
	}

	//Check if the magic characters are "CAFF"
	if (std::string(magic, sizeof(magic)) != "CAFF") {
		std::cerr << "Magic is not CAFF" << std::endl << "Magic: " << magic << std::endl;
		return false;
	}
	//Check if the header size is equal to 20 (magic(4) + header_size(8) + num_anim(8))
	if (header_size != 20) {
		std::cerr << "Header size is not correct" << std::endl << "Header size: " << header_size << std::endl;
		return false;
	}
	//Check if there are CIFFs to parse
	if (num_anim < 1) {
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
	uint16_t year = 0;
	uint8_t month = 0;
	uint8_t day = 0;
	uint8_t hour = 0;
	uint8_t minute = 0;
	//Creator length integer
	size_t creator_length = 0;

	//Read date and creator length
	if (!file.read(reinterpret_cast<char*>(&year), sizeof(year))) {
		std::cerr << "Failed to read file!" << std::endl;
		return false;
	}
	if (!file.read(reinterpret_cast<char*>(&month), sizeof(month))) {
		std::cerr << "Failed to read file!" << std::endl;
		return false;
	}
	if (!file.read(reinterpret_cast<char*>(&day), sizeof(day))) {
		std::cerr << "Failed to read file!" << std::endl;
		return false;
	}
	if (!file.read(reinterpret_cast<char*>(&hour), sizeof(hour))) {
		std::cerr << "Failed to read file!" << std::endl;
		return false;
	}
	if (!file.read(reinterpret_cast<char*>(&minute), sizeof(minute))) {
		std::cerr << "Failed to read file!" << std::endl;
		return false;
	}
	if (!file.read(reinterpret_cast<char*>(&creator_length), sizeof(creator_length))) {
		std::cerr << "Failed to read file!" << std::endl;
		return false;
	}

	//Check if the date format is correct
	if (year > 9999) {
		std::cerr << "Year is not correct!" << std::endl << "Year: " << year << std::endl;
		return false;
	}
	if (month < 1 || month > 12) {
		std::cerr << "Month is not correct!" << std::endl << "Month: " << month << std::endl;
		return false;
	}
	if (day < 1 || day > 31) {
		std::cerr << "Day is not correct!" << std::endl << "Day: " << day << std::endl;
		return false;
	}
	if (hour > 24) {
		std::cerr << "Hour is not correct!" << std::endl << "Hour: " << hour << std::endl;
		return false;
	}
	if (minute > 60) {
		std::cerr << "Minute is not correct!" << std::endl << "Minute: " << minute << std::endl;
		return false;
	}
	//Check if the creator matches up with the CAFF block length
	if (creator_length != credits_length - 14) {
		std::cerr << "Creator length mismatch!" << std::endl << "Creator length is: " << creator_length << " when it should be: " << credits_length - 14 << std::endl;
		return false;
	}
	//If there is no creator return and parsing can continue
	if (creator_length != 0) {
		//Create char array for the creator string
		char* creator = new char[creator_length];

		//Read in the creator string
		if (!file.read(creator, std::streamsize(creator_length))) {
			std::cerr << "Failed to read file!" << std::endl;
			delete[] creator;
			return false;
		}
		//Convert to string
		std::string screator(creator, creator_length);
		delete[] creator;

		//Print the creator
		std::cout << "CAFF Creator: " << screator << std::endl;
	}
	//Print creation time
	std::cout << "Creation date: " << year << "." << static_cast<int>(month) << "." << static_cast<int>(day) << ". " << static_cast<int>(hour) << ":" << static_cast<int>(minute) << std::endl;
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
	size_t header_size = 0;
	//CIFF content size
	size_t content_size = 0;
	//CIFF image width
	size_t width = 0;
	//CIFF image height
	size_t height = 0;

	//Read in the header fields
	if (!file.read(magic, sizeof(magic))) {
		std::cerr << "Failed to read file!" << std::endl;
		return false;
	}
	if (!file.read(reinterpret_cast<char*>(&header_size), sizeof(header_size))) {
		std::cerr << "Failed to read file!" << std::endl;
		return false;
	}
	if (!file.read(reinterpret_cast<char*>(&content_size), sizeof(content_size))) {
		std::cerr << "Failed to read file!" << std::endl;
		return false;
	}
	if (!file.read(reinterpret_cast<char*>(&width), sizeof(width))) {
		std::cerr << "Failed to read file!" << std::endl;
		return false;
	}
	if (!file.read(reinterpret_cast<char*>(&height), sizeof(height))) {
		std::cerr << "Failed to read file!" << std::endl;
		return false;
	}

	//Check if magic characters are CIFF
	if (std::string(magic, sizeof(magic)) != "CIFF") {
		std::cerr << "Magic is not CIFF" << std::endl << "Magic: " << magic << std::endl;
		return false;
	}

	//Check if the header size is at least 36 characters
	if (header_size <= 36) {
		std::cerr << "Header size is incorrect" << std::endl << "Header size: " << header_size << std::endl;
		return false;
	}

	//Check if the Content size is width * height * 3
	if (content_size != width * height * 3) {
		std::cerr << "Content size is incorrect!" << "Content size: " << content_size << " != " << width << " * " << height << " * " << "3" << std::endl;
		return false;
	}

	//Check if there are pixels to convert
	if (content_size == 0) {
		std::cerr << "No pixels to make JPEG!" << std::endl;
		return false;
	}

	//Calculate the remaining size of the header
	size_t remaining_header_size = header_size - 36;

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
	if (!canReadBytes(file, file.tellg(), content_size)) {
		std::cerr << "Not enough bytes left in file!" << std::endl;
		return false;
	}

	//Make the buffer for the JPEG content
	char* buffer = new char[content_size];
	if (!file.read(buffer, content_size)) {
		std::cerr << "Failed to read file!" << std::endl;
		return false;
	}

	//Make the file name
	std::string name = fileName + ".jpg";

	//Make the file.
	int result = stbi_write_jpg(name.c_str(), (int)width, (int)height, 3, buffer, 50);

	//If the result is 0 it was not successful
	if (result == 0) {
		std::cerr << "Failed to make JPEG file!" << std::endl;
		return false;
	}
	//Print CIFF data
	std::cout << "CIFF size: " << width << " x " << height << std::endl;
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
	size_t duration = 0;

	//Read in the duration
	if (!file.read(reinterpret_cast<char*>(&duration), sizeof(duration))) {
		std::cerr << "Failed to read file!" << std::endl;
		return false;
	}

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

	//Read the first block header
	std::optional<CAFFBlockHeader> firstBlockOpt = readCAFFBlockHeader(file);

	//If something went wrong return with false
	if (!firstBlockOpt.has_value()) {
		std::cerr << "Failed to parse CAFF Block!" << std::endl;
		return false;
	}

	//Get the value of the first block
	CAFFBlockHeader firstBlock = firstBlockOpt.value();

	//Check if the first block is a header block
	if (firstBlock.id != CAFFBlockType::header) {
		std::cerr << "The first block was not a header block!" << std::endl;
		return false;
	}

	if (!readCAFFHeaderBlock(file)) {
		std::cerr << "Failed to parse CAFF Header Block!" << std::endl;
		return false;
	}

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
			std::cerr << "Multiple Header Blocks in the file!" << std::endl;
			return false;
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
