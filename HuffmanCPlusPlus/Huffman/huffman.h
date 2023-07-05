#pragma once

#include <type_traits>		// Check T is char or byte thru is_same
#include <unordered_map>	// Map char to freq: size_t; codebook: string
#include <queue>			// priority_queue to sort by freq
#include <fstream>
#include <string>

namespace Huffman {

// Do: Tokenize data in file into discrete bytes
template<typename T = unsigned char>
std::basic_string<T> readFile(const std::string& readPath);

template<typename T = unsigned char>
struct Node {
	static_assert(std::is_same<char, T>::value || std::is_same<unsigned char, T>::value,
		"T must be char (text) or unsigned char (binary)");

	T c;

	// Store actual count, not as proportion of text.size()
	size_t freq;
	Node   *left, *right;

	// If node.right (ie node has any child), it encodes merge
	Node(T c = '\0', size_t freq = 0,
		Node* left = nullptr, Node* right = nullptr) :
		c(c), freq(freq), left(left), right(right) {}

	/* // Do: Check tree is properly encoded (read back same as written)
	bool operator==(const Node& other) {
		if (this->c    != other.c)    return false;
		if (this->freq != other.freq) return false;

		if (this->left && other.left) {
			if (!(*(this->left ) == *(other.left ))) return false;
		}
		else if !(!this->left && !other.left) return false;

		if (this->right && other.right) {
			if (!(*(this->right) == *(other.right))) return false;
		}
		else if !(!this->right && !other.right) return false;

		return true;
	};*/

	~Node() { delete left; delete right; }
};

// Do: Resolve nodes of equal freq: recursively compare
//	   descendants' freq. At end, compare leaf nodes' c
template<typename T>
struct NodeComparator {
	bool operator()(Node<T>* n1, Node<T>* n2) {
		while (true) {
			if (n1->freq != n2->freq) {
				return n1->freq > n2->freq;
			}

			if (n1->right && n2->right) {
				n1 = n1->right;
				n2 = n2->right;
			}
			else break; 
		}
		return n1->c > n2->c;
	}
};

template<typename T = unsigned char>
struct Coder {
	static_assert(std::is_same<char, T>::value || std::is_same<unsigned char, T>::value,
		"T must be char (text) or unsigned char (binary)");

	std::basic_string<T> text, decoded;

	// For encoded, can use specialized vector<bool>,
	// which stores bit data as actual bit, to optimize
	// space but at cost of speed as reading whole byte 
	// is more efficient than reading 1 bit
	// For that route, also change codebook's type
	std::string			encoded, encodePath;
	Node<T>*			tree = nullptr;
	std::unordered_map<T, size_t>	   freqs;
	std::unordered_map<T, std::string> codebook;

	// nBytes needed to store both encoded and tree data
	size_t usage = 0;

	Coder(const std::basic_string<T>& text) : text(text) {
		encode();
	}

	Coder( // Also write tree and encoded data to encodePath
		const std::basic_string<T>& text,
		const std::string& encodePath):
		text(text), encodePath(encodePath) {
		encode();
	}

	// To: Compute components and encode thru codebook
	// 1. Count char freq, normalizing freq < 256 (1 byte)  
	// 2. Build tree with which to fill codebook
	// 3. Into path, write tree and encoded data
	// 4. Set usage = nBytes needed for tree and encoded tree
	void encode();

	// To: Write decoded content to decodePath
	// 
	// Must be called explicitly, unlike encode()
	std::basic_string<T>& decode(
		const std::string& decodePath = "");

	~Coder() { delete tree; }
};

// Re: Tree thru which to use to fill codebook (encode)
template<typename T>
Node<T>* getTree(
	const std::unordered_map<T, size_t>& freqs,
	std::unordered_map<T, std::string> & codebook);

// Re: Tree to use to decode (need codebook only to encode)
template<typename T>
Node<T>* getTree(
	const std::unordered_map<T, size_t>& freqs);

// Do: Traverse tree: for char, put its encoding into codebook
//
// Does not verify node != null. getTree(..) should had verified
template<typename T>
void fillCodebook(
	std::unordered_map<T, std::string>& codebook,
	Node<T>* node,
	std::string& bitstr);

// Re: Original text, to also write to non-"", valid writePath
template<typename T>
std::basic_string<T> decode(
	Node<T>* tree,
	const std::string& bitstr, const std::string& writePath = "");

// Re: Original text, to also write to non-"", valid writePath
template<typename T = unsigned char>
std::basic_string<T> decode(
	const std::string& readPath, const std::string& writePath = "");

// Re: # of bytes written to binary file. If fail, 0
// To: Write char-freq data to use later to rebuild same tree
// 
// If > 127 chars in map, write only freq, put freq = 0 for
// those chars not present, use index to match freq to char
template<typename T>
size_t writeTree(
	const std::unordered_map<T, size_t>& freqs,
	const std::string				   & treePath);

// Re:  Node/tree derived from char freq data in treePath
// sep: Place at which to read encoded data if in
//		treePath file, encoded data follow tree data
template<typename T = unsigned char>
Node<T>* readTree(
	const std::string& treePath, size_t* sep = nullptr);

// Re: # of bytes written to file, from string<0|1> padded to 8
// toAppend: If true, append encoded to file already holding tree data
size_t writeEncoded(
	const std::string& bitstr, const std::string& writePath,
	bool toAppend = false);

// Re: string of 0s and 1s, to decode thru tree
// sep != 0: Start of encoded data if tree data precedes in same file
std::string readEncoded(
	const std::string& readPath, size_t sep = 0);
 
// Do: Pad final block of < 8 bits to 8 bits  
std::vector<char> bitsToBytes(const std::string& bitstr);

// .inl extends .h and #include directly copies and pastes 
// .inl's contents onto .h. Template must be available at
// compile-time--thus in h. or .inl derivative--to be used to
// generate concrete classes with specific types at run-time
// Need to remove .inl from project and (re)Add Existing Item

#include "huffman.inl"
} // namespace Huffman closed