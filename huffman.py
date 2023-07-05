from collections import defaultdict
import heapq
from typing import Dict
from math import ceil
from bitarray import bitarray # https://pypi.org/project/bitarray/
import os
 
class Node:
    def __init__(self, c = 0, freq = 0, left = None, right = None):
        
        # For Python, int needs fewer bytes to store than str/char
        self.c    : int = c
        
        # Store actual count, not as proportion of len(text)
        self.freq : int = freq
        
        # If node.right (node has any child), it encodes merge
        self.left : Node = left
        self.right: Node = right
    
    # Compare in order by freq, c, right descendant's c
    def __lt__(self, oth):
        if self.freq != oth.freq:
            return self.freq < oth.freq
        
        # Compare branches for decisive compare between merges of
		# == freq. Given chars cannot repeat, recursion will end
        c1, c2 = self, oth
        while c1.right and c2.right:
            c1 = c1.right
            c2 = c2.right
            
        return c1.c < c2.c
    
    def __repr__(self):
        return f"Node({self.c}, {self.freq}, {self.left}, {self.right})"
    
class Coder:
    def __init__(self, text, encodePath = ""):
        if isinstance(text, str): # Convert
            text = bytes(text, encoding = "utf-8")
                                                         
        self.text      : bytes = text
        self.encodePath= encodePath
        
        # Non-native class to hold str of bits that synergizes with bytes
        self.encoded   = bitarray()
        self.decoded   = bytearray()
        self.tree      : Node  = None
        self.freqs     : Dict[int, int]      = defaultdict(int)
        self.codebook  : Dict[int, bitarray] = dict()
        
        # nBytes needed to store both encoded and tree
        self.usage  : int   = 0
        
        # Run: fill encoded, decoded, tree, freqs, codebook, usage
        self.encode()
        
    '''Fill components needed to encode, return encoded: str<bit>
       1. Count freqs of chars, normalizing freq < 256 (1 byte)
       2. Build Huffman tree and use it to fill codebook
       3. Into valid path, write tree and encoded data
       4. Even for invalid path and no write, calculate nBytes
          that would be written'''
    def encode(self) -> None:
        if not self.text: return
        
        for c in self.text: # 1
            self.freqs[c] += 1
            
        # If any freq needs more than 1 byte to store, cut all
        # by // maxFreq so every freq needs only 1 byte
        maxFreq = max(self.freqs.values())
        if maxFreq > 255:
            if len(self.freqs) > 127:
                for c, freq in self.freqs.items():
                    self.freqs[c] = 255 * freq // maxFreq   
            else:
                for c, freq in self.freqs.items():
                    self.freqs[c] = max(255 * freq // maxFreq, 1)
                
        self.tree = getTree(self.freqs, self.codebook) # 2
        
        for c in self.text:
            self.encoded.extend(self.codebook[c])
        
        self.usage  = writeTree(self.freqs, self.encodePath)
        self.usage += writeEncoded(self.encoded, self.encodePath, True)
        
    # Needs to be called explicitly, unlike encode()
    def decode(self, decodePath = "") -> bytearray():
        self.decoded = decode(self.tree, self.encoded, decodePath)
        return self.decoded
    
#============================================================       
def getTree(freqs, codebook = None) -> Node:
    heap = [Node(c, freq) for (c, freq) in freqs.items()]
    heapq.heapify(heap)
    
    while len(heap) > 1:
        right = heapq.heappop(heap)
        left  = heapq.heappop(heap)
        
        freqSum = right.freq + left.freq
        heapq.heappush(heap, Node(0, freqSum, left, right))
        
    root = heapq.heappop(heap)
    
    if codebook is not None:
        # For non-native bitarray, big-endian ensures padding is to back
        bitstr = bitarray(endian = "big")
        fillCodebook(codebook, root, bitstr)
    
    return root

# Does not check if node is non-None. If called by
# getTree(..) or recursively, node will be non-None
def fillCodebook(codebook: Dict[int, bitarray], node, bitstr: bitarray) -> None:
    if not node.right:
        codebook[node.c] = bitstr.copy()
        
        
	# Assign 0|1 bit to left|right, recurse down children 
	# Once child branches finish, this recursive call also 
	# completes and call stack auto backtracks, so remove
	# added bit, being imprint of this branch
    else:
        bitstr.append(0)
        fillCodebook(codebook, node.left, bitstr)
        
        bitstr[-1] = 1
        fillCodebook(codebook, node.right, bitstr)
        
        bitstr.pop()
        
# tree + encoded -> decoded. Write decoded to writePath
def decode(tree: Node, encoded: bitarray, writePath = "") -> bytearray:
    
    # Overload decode(readPath: str, writePath: str). Give readPath 
    # args[0]: label tree; give writePath args[1], label encoded
    if isinstance(tree, str):
        readPath = tree
        writePath = encoded
        sep = [0]
        tree    = readTree   (readPath, sep) 
        encoded = readEncoded(readPath, sep) 
    
    decoded = bytearray()
    if not encoded or not tree: return decoded
    
    current = tree
    for bit in encoded:
        current = current.right if bit else current.left
        if not current.right:
            decoded.append(current.c)
            current = tree
            
    if writePath:
        with open(writePath, "wb") as file:
            file.write(decoded)
            
    return decoded

# If > 127 distinct chars, write only freq, use index to
# retrieve char, pad those chars not present with freq == 0
def writeTree(freqs: Dict[int, int], treePath: str) -> int:
    if not freqs or not treePath: return 0
    
    with open(treePath, mode = "wb") as file:
        n = min(len(freqs), 255)
        file.write(n.to_bytes(1, byteorder = "big"))
        
        if n > 127:
            byts = bytearray(256 * b'\x00')
            for i in range(0, 256):
                if i in freqs:
                    byts[i] = freqs[i] 
            n = 256
            
        else:
            byts = bytearray(2 * n * b'\x00')
            n = 0
            for c, freq in freqs.items():
                byts[n] = c   ; n += 1
                byts[n] = freq; n += 1
                
        file.write(byts)
        return n + 1
    
# From tree data, restore freqs table, pass to getTree(freqs)
# For file holding both tree, encoded data, sep relays
# position tree data finishes and encoded data starts 
# sep: Wrap int in object so function can persist int update
#      by modifying wrapper rather than int directly
def readTree(treePath: str, sep = [0]) -> Node:
    with open(treePath, mode = "rb") as file:
        freqs = dict()
        
        n = int.from_bytes(file.read(1), "big")
        
    	# If count of distinct chars in text > 127, more
        # efficient to not store char key and rather associate
        # char by its index. Any char index with freq == 0 was 
        # padded and not present
        if n > 127:
            for i in range(0, 256):
                freq = int.from_bytes(file.read(1), "big")
                if  freq > 0:
                    freqs[i] = freq
                    
        else:
            for i in range(0, n):
                c    = int.from_bytes(file.read(1), "big")
                freq = int.from_bytes(file.read(1), "big")
                freqs[c] = freq
                
        sep[0] = 1 + min(2 * len(freqs), 256)
        return getTree(freqs)

# Return total bytes written to binary file. If fail, return 0
def writeEncoded(encoded: bitarray, writePath: str, toAppend = False) -> int:
    if not encoded or not writePath: return 0
    
    with open(writePath, mode = ("ab" if toAppend else "wb")) as file:
        byts = bitsToBytes(encoded)
        file.write(byts)
        return len(byts) + 1
            
    return 0

def bitsToBytes(bitstr: bitarray) -> bytearray:
    # 8 - nLeftover to get nBits needed to pad to 8 
    # If bitstr already has len of 8 multiple, need 0
    nPad = 8 - ((len(bitstr) - 1) % 8 + 1)
    
    n = (len(bitstr) + nPad + 8) // 8
    bytArray     = bytearray(b'x\00' * n)
    
    # Metadata byte at front tells nPad(ded) at back
    bytArray[0]  = nPad
    bytArray[1:] = bitstr.tobytes()
    
    return bytArray
    
def readEncoded(readPath, sep = [0]) -> bitarray:
    with open(readPath, "rb") as file:
        file.seek(sep[0])
        byts = file.read()
        bitstr = bitarray(endian = "big")
        bitstr.frombytes(byts[1:])
    
        for i in range(byts[0]):
            bitstr.pop()
            
        return bitstr
    
def TestFile(readPath = "./lipsum.txt"):
    if not os.path.exists("./generated"):
        os.mkdir("./generated")
        
    name, ext = os.path.splitext(readPath)
    stem = "./generated/" + name
    
    with open(readPath, "rb") as file:
        coder = Coder(file.read())
        
        # 1: Core Huffman: decode through class
        coder.decode(stem + "_decoded" + ext)
        
        # 2: Write tree, encoded to separate files, decode from files
        #writeTree(coder.freqs, stem + ".ser")
        #writeEncoded(coder.encoded, stem + ".dat")
        #decode(readEncoded(stem + ".dat"), readTree(stem + ".ser"), stem + "_decoded" + ext)
        
        # 3: Write tree, append encoded to that same file
        writeTree(coder.freqs, stem + ".bin")
        writeEncoded(coder.encoded, stem + ".bin", True)
        decode(stem + ".bin", stem + "_decoded" + ext)

if __name__ == "__main__":
    TestFile("./png.png")
    




