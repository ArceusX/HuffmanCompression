template<typename T>
std::basic_string<T> Huffman::readFile(const std::string& readPath) {
	// Set cursor initially at end
	std::ifstream file(readPath, std::ios::binary | std::ios::ate);
	if (!file) return std::basic_string<T>();

	// Measure distance between start and cursor (at end)
	auto size = file.tellg();
	std::basic_string<T> str(size, '0');
	
	// Set cursor at start for reading
	file.seekg(std::ios::beg);
	file.read((char*)& str[0], size);
	return str;
}

template<typename T>
void Coder<T>::encode() {
	if (text.empty()) return;				   // 1
	for (const T& c : text) freqs[c]++;
	 
	size_t maxFreq = std::max_element(freqs.begin(), freqs.end(),
		[](auto& p1, auto& p2) {
			return p1.second < p2.second; })->second;

	// If any freq needs more than 1 byte to store, / maxFreq for
	// all to bound to [0, 255] so every freq needs only 1 byte
	if (maxFreq > 255) {
		if (freqs.size() > 128) {
			for (auto& [c, freq] : freqs) {
				freq = 255 * freq / maxFreq;

				// Distinguish c of normalized-0 freq and non-present c
				freq = (freq < 1 ? 1 : freq);
			}
		}
		else {
			for (auto& [c, freq] : freqs) {
				freq = 255 * freq / maxFreq;
			}
		}
	}

	tree = getTree<T>(freqs, codebook);		 // 2

	// For freqs.size() ~ 256, file is likely binary, so
	// most chars need 8 (= log2(256)) bits to encode
	// For English corpus, most chars need average of 5 bits 
	encoded.reserve(text.size() * 5 * (freqs.size() > 230 ? 1.6 : 1));
	for (const T& c : text) encoded.append(codebook[c]);

	writeTree(freqs, encodePath);			 // 3
	writeEncoded(encoded, encodePath, true); // True: Do append

	usage = 1 + 2 * ((freqs.size() > 128) ? 128 : freqs.size()); // 4
	usage += 1 + ((encoded.size() + 7) / 8);
}

template<typename T>
Node<T>* getTree(
	const std::unordered_map<T, size_t>& freqs,
	std::unordered_map<T, std::string> & codebook) {

	Node<T>* root = getTree<T>(freqs);

	// Recursive calls of fillCodebook() extend shared str<0|1>
	std::string bitstr("");
	fillCodebook(codebook, root, bitstr);
	return root;
}

template<typename T>
Node<T>* getTree(const std::unordered_map<T, size_t>& freqs) {
	if (freqs.empty()) return nullptr;

	std::priority_queue<Node<T>*, std::vector<Node<T>*>,
		NodeComparator<T>> heap;

	for (auto& [c, freq] : freqs) {
		heap.push(new Node<T>(c, freq));
	}

	// While nodes to merge, pop pair of lowest-freqs, assign them
	// to parent of their freqs summed, push that parent into heap
	while (heap.size() > 1) {
		Node<T>* right = heap.top(); heap.pop();
		Node<T>* left  = heap.top(); heap.pop();

		size_t freqSum = left->freq + right->freq;
		heap.push(new Node<T>('\0', freqSum, left, right));
	}

	return heap.top(); // Only root remains
}

template<typename T>
void fillCodebook(
	std::unordered_map<T, std::string>& codebook,
	Node<T>* node,
	std::string& bitstr) {

	// Leaf nodes hold chars, rather than merges
	if (!node->right) {
		codebook.emplace(node->c, bitstr);
	}

	// Recurse: push 0|1 for left|right. Remove on backtrack 
	else {
		bitstr.push_back('0');
		fillCodebook(codebook, node->left,  bitstr);

		bitstr.back() = '1';
		fillCodebook(codebook, node->right, bitstr);

		bitstr.pop_back();
	}
}

template<typename T>
std::basic_string<T>& Coder<T>::decode(const std::string& decodePath) {
	decoded = Huffman::decode(tree, encoded, decodePath);
	return decoded;
}

template<typename T>
std::basic_string<T> decode(
	const std::string& readPath, const std::string& writePath) {
	size_t   sep = 0;
	Node<T>* tree = readTree<T>(readPath, &sep);
	return decode(tree, readEncoded(readPath, sep), writePath);
}

template<typename T>
std::basic_string<T> decode(
	Node<T>* tree,
	const std::string& bitstr, const std::string& writePath) {
	if (!tree || bitstr.empty()) return std::basic_string<T>();

	// 8 bits to encode binary data; ~5 to encode English text
	std::basic_string<T> decoded;
	decoded.reserve(
		bitstr.size()/5/(std::is_same<unsigned char, T>::value ? 1.6 : 1));

	Node<T>* current = tree;
	for (const char& bit : bitstr) {
		current = (bit == '1') ? current->right : current->left;
		if (!current->right) {
			decoded.push_back(current->c);
			current = tree;
		}
	}

	std::ofstream file(writePath, std::ios::binary);
	if (file) file.write((char*)&decoded[0], decoded.size());

	return decoded;
}

template<typename T>
size_t writeTree(
	const std::unordered_map<T, size_t>& freqs,
	const std::string& treePath) {

	if (freqs.empty()) return 0;
	std::ofstream file(treePath, std::ios::binary);
	if (!file) return 0;

	size_t n = freqs.size() > 128 ? 256 : freqs.size();
	file.write((char*) &n, 1);     // 1 metadata byte

	if (n > 128) {
		n = 0;
		for (size_t i = 0; i < 256; i++) {
			auto p = freqs.find(T(i));

			if (p == freqs.end()) {// Write n which is 0
				file.write((char*)&n, 1);
			}
			else file.write((char*)&(p->second), 1);
		}
		n = 256;
	}
	else {
		for (auto& [c, freq] : freqs) {
			file << c;
			file.write((char*)&freq, 1);
		}
		n *= 2;
	}

	return n + 1; // + 1: metadata byte
}

template<typename T>
Node<T>* readTree(const std::string& treePath, size_t* sep) {
	std::ifstream file(treePath, std::ios::binary);
	if (!file) return nullptr;

	std::unordered_map<T, size_t> freqs;
	char c, f;

	file.get(c);
	size_t n = (unsigned char) c;

	// If text's # of distinct chars > 128, saves
	// space to not store char key and match freq to
	// char by index instead. Any char index with 
	// freq == 0 was padded and not present in text
	if (n > 128) {
		for (size_t i = 0; i < 256; i++) {
			file.get(f);
			if ((n = (size_t)(unsigned char)f) != 0) {
				freqs.insert({ T(i), n });
			}
		}
	}
	else {
		for (size_t i = 0; i < n; i++) {
			file.get(c);
			file.get(f);
			freqs.insert({ c, (size_t)(unsigned char)f });
		}
	}

	if (sep) {
		*sep = 1 + ((freqs.size() > 128) ? 256 : 2 * freqs.size());
	}

	return getTree<T>(freqs);
}

size_t writeEncoded(
	const std::string& bitstr,
	const std::string& writePath,
	bool toAppend) {
	if (bitstr.empty()) return 0;

	std::ofstream file(writePath, std::ios::binary |
		(toAppend ? std::ios::app : std::ios::out));

	if (!file) return 0;

	std::vector<char> bytes = bitsToBytes(bitstr);

	// Write nBytes read from &bytes[0] (1 char: 1 byte)
	file.write((char*) &bytes[0], bytes.size());

	return bytes.size() + 1; // + 1: '\0' written to end 
}

std::string readEncoded(
	const std::string& readPath, size_t sep) {
	std::ifstream file(readPath, std::ios::binary | std::ios::ate);
	if (!file) return std::string();
	size_t usage = (size_t) file.tellg() - sep;
	file.seekg(sep);

	// 1st byte is size_t metadata distinct from text
	char c;
	file.get(c);
	size_t nPad = (size_t) c;

	std::string bitstr(8 * (usage - 1), '0'); // 8 bits to byte
	size_t n = 0;
	while (file.get(c)) {
		for (size_t i = 0; i < 8; i++, n++) {
			// For ith bit from right, right-shift by (7 - i)
			bitstr[n] = (c >> (7 - i) & 1) ? '1' : '0';
		}
	}

	bitstr.erase(bitstr.size() - nPad, nPad);
	return bitstr;
}

std::vector<char> bitsToBytes(const std::string& bitstr) {
	// 8 - nLeftover: nBits needed to pad to 8
	// If len is already 8-multiple, need 0 
	size_t nPad = 8 - ((bitstr.size() - 1) % 8 + 1);

	// Metadata byte at front tells nPad(ded) at back
	std::vector<char> bytes(1, nPad);
	bytes.reserve((bitstr.size() + nPad) / 8 + 2);

	// i: Up to, excluding, highest 8-multiple < bitstr.size() 
	size_t i = 0;
	if (bitstr.size() >= 8) {
		for (size_t n = bitstr.size() - 8; i <= n; i = i + 8) {
			bytes.push_back(std::stoi(bitstr.substr(i, 8), 0, 2));
		}
	}

	// ie If nPad == 3, pad on right of (8 - 3 = 5 leftover) bits 
	if (nPad != 0) {
		bytes.push_back(std::stoi(
			bitstr.substr(i, 8 - nPad) + std::string(nPad, '0'), 0, 2));
	}
	return bytes;
}