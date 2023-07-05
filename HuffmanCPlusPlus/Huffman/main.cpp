#include <stdio.h>
#include <filesystem>

#if PR_DEBUG == 0
#define CMD() cmd(argc, argv)
#else
#define CMD()
#endif

#include "huffman.h" 
using namespace Huffman;

// On Debug, uncomment Test(..) definition, uncomment all
// sections except desired one (1, 2, or 3), uncomment Test(..) 
// call in main(), check output files in /generated folder

void Test(const std::string& readPath = "./lipsum.txt") {
	if (!std::filesystem::exists("./generated")) {
		std::filesystem::create_directory("./generated");
	}

	std::filesystem::path path(readPath);
	auto name = path.stem().string();
	auto stem = std::filesystem::path("./generated/" + name).string();
	auto ext  = path.extension().string();

	Coder coder(readFile(readPath), "./generated/" + name+ ".dat");

	// 1: Core Huffman: decode through class
	//coder.decode(stem + "_de" + ext);

	// 2: Write tree, encoded to separate files, decode from files
	writeTree(coder.freqs, stem + ".ser");
	writeEncoded(coder.encoded, stem + ".dat");
	decode(readTree(stem + ".ser"), readEncoded(stem + ".dat"), stem + "_de" + ext);

	// 3: Write tree, append encoded to that same file
	//writeTree(coder.freqs, stem + ".bin");
	//writeEncoded(coder.encoded, stem + ".bin", true); // toAppend : true
	//decode(stem + ".bin", stem + "_de" + ext);

	size_t txSize = coder.text.size();

	printf(
		"--> Encoded %s (%u bytes) to %u bytes: %.3f compression ratio\n",
		(name + ext).c_str(), txSize, coder.usage, (long float)txSize / coder.usage);
}

void cmd(int argc, char* argv[]) {
	if (argc == 1 || (argc == 2 && (strcmp(argv[0], "-h") == 0))) {
		printf(
			"Huffman: -h to print this help message\nCommands:\n"
			"    -en[code] readPath encodePath {3 args}\n"
			"    -de[code] readPath decodePath {3 args}\n\n"
			"Output : Compression ratio\n\n");
	} 
	else if (argc == 4) {
		if (strcmp(argv[1], "-encode") == 0 || strcmp(argv[1], "-en") == 0) {

			Coder<unsigned char> coder(readFile<unsigned char>(argv[2]), argv[3]);
			size_t txSize = coder.text.size();
			if (coder.text.size()) {
				printf(
					"--> Encoded %s (%u bytes) to %s (%u bytes) : %.3f compression ratio\n",
					argv[2], txSize, argv[3], coder.usage, (long float)txSize / coder.usage);
			}
			else printf("Cannot open file %s", argv[2]);
		}
		else if (strcmp(argv[1], "-decode") == 0 || strcmp(argv[1], "-de") == 0) {

			if (size_t txSize = decode<unsigned char>(argv[2], argv[3]).size()) {
				size_t usage = 0;
				std::ifstream file(argv[2], std::ios::binary | std::ios::ate);
				usage = file.tellg();

				printf(
					"--> Decoded %s (%u bytes) to %s (%u bytes) : %.3f compression ratio\n",
					argv[2], usage, argv[3], txSize, (long float)txSize / usage);
			}
			else printf("Cannot open file %s", argv[2]);
		}
		else printf("Error: Cannot process command %s", argv[1]);
	}
	else printf("Error: Incorrect number of arguments");
}

int main(int argc, char* argv []) {
	CMD(argc, argv);
	Test("./lipsum.txt"); // Release build: Comment out
}