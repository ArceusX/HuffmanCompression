

## Features

1. Encode/compress data or decode, from or to files

2. Coder class encodes to a file. You can then call decode() on file thru that Coder to get original text 

## Usage

```diff
- std::string stem("./png")
- std::string ext(".png")
- Coder coder(readFile(stem + name))

+ // 1: Core Huffman: decode through class
+ coder.decode(stem + "_decoded" + ext);

! // 2: Write tree, encoded to separate files, decode from files
! writeTree(coder.freqs, stem + ".ser");
! writeEncoded(coder.encoded, stem + ".dat");
! decode(readTree(stem + ".ser"), readEncoded(stem + ".dat"), stem + "_decoded" + ext);

+ // 3: Write tree, append encoded to that same file
+ writeTree(coder.freqs, stem + ".bin");
+ writeEncoded(coder.encoded, stem + ".bin", true); // toAppend : true
+ decode(stem + ".bin", stem + "_decoded" + ext);
```
## Functions
| Core (# overloads)          | Helper                |
|:---------------------------:|-----------------------|
| encode                      | fillCodebook          |
| decode  (2)                 | bitsToBytes           |
| getTree (2)                 | readFile              |               
| (write/read)(Tree/Encoded)  |                       |

### In-Depth

| Function     | Description                                                               |
|--------------|---------------------------------------------------------------------------|
| encode       | Fill components (freqs->tree->codebook) and encode thru codebook          |
| decode       | Thru tree, decode string<1|0>, obtainable from files. Write to path       |
| getTree      | From map<char -> freq>, build tree and use tree to fill codebook          |
| writeTree    | Write data in char-freq map to path                                       |
| readTree     | Read data in path into char-freq map. Call getTree(such map)              |
| writeEncoded | Write bytes, from string<0|1> padded to 8                                 |
| readEncoded  | Read into string<0|1> from data in path                                   |
| fillCodebook | Traverse node/tree: for char, put its encoding into codebook              |
| bitsToBytes  | Pad final block of < 8 bits to 8 bits                                     |
| readFile     | Tokenize data in file into discrete bytes                                 |

### Defined in CMD() for Release Build
```diff
+ Huffman: -h to print this help message
- Commands:
!   -en[code] readPath encodePath {3 args}
!   -de[code] readPath decodePath {3 args}

+ Output : Compression ratio

// Example run
---> Encoded lipsum.txt (2958 bytes) to 1656 bytes: 1.786 compression ratio
```
