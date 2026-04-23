#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstring>
#include <cstdint>
#include <sys/stat.h>
#include <unordered_map>

const int NUM_BUCKETS = 20;
const std::string DATA_DIR = "data";

struct Entry {
    std::string index;
    std::vector<int> values;
};

std::vector<Entry> bucketCache[NUM_BUCKETS];
bool cacheDirty[NUM_BUCKETS] = {false};

unsigned int hashIndex(const std::string& s) {
    unsigned int h = 0;
    for (char c : s) {
        h = h * 31 + (unsigned char)c;
    }
    return h % NUM_BUCKETS;
}

std::string getBucketFilename(int bucket) {
    return DATA_DIR + "/bucket" + std::to_string(bucket) + ".dat";
}

void ensureDataDir() {
    struct stat st;
    if (stat(DATA_DIR.c_str(), &st) != 0) {
        mkdir(DATA_DIR.c_str(), 0755);
    }
}

void loadBucket(int bucket) {
    if (!bucketCache[bucket].empty()) {
        return;
    }
    std::string filename = getBucketFilename(bucket);
    std::ifstream infile(filename, std::ios::binary);
    if (!infile.is_open()) {
        return;
    }

    while (infile.peek() != EOF) {
        uint16_t index_len;
        if (!infile.read(reinterpret_cast<char*>(&index_len), sizeof(index_len))) break;
        if (index_len > 100) break;

        std::string index(index_len, '\0');
        if (!infile.read(&index[0], index_len)) break;

        uint32_t value_count;
        if (!infile.read(reinterpret_cast<char*>(&value_count), sizeof(value_count))) break;
        if (value_count > 100000) break;

        std::vector<int> values(value_count);
        for (uint32_t i = 0; i < value_count; i++) {
            int32_t v;
            if (!infile.read(reinterpret_cast<char*>(&v), sizeof(v))) {
                values.clear();
                break;
            }
            values[i] = v;
        }
        if (values.empty()) break;

        bucketCache[bucket].push_back({index, values});
    }
    infile.close();
}

void saveBucket(int bucket) {
    if (!cacheDirty[bucket]) {
        return;
    }
    std::string filename = getBucketFilename(bucket);
    std::ofstream outfile(filename, std::ios::binary);
    if (!outfile.is_open()) {
        return;
    }

    for (const auto& entry : bucketCache[bucket]) {
        uint16_t index_len = static_cast<uint16_t>(entry.index.size());
        outfile.write(reinterpret_cast<const char*>(&index_len), sizeof(index_len));
        outfile.write(entry.index.data(), index_len);

        uint32_t value_count = static_cast<uint32_t>(entry.values.size());
        outfile.write(reinterpret_cast<const char*>(&value_count), sizeof(value_count));

        for (int v : entry.values) {
            int32_t iv = v;
            outfile.write(reinterpret_cast<const char*>(&iv), sizeof(iv));
        }
    }
    outfile.close();
    cacheDirty[bucket] = false;
}

void flushAllBuckets() {
    for (int i = 0; i < NUM_BUCKETS; i++) {
        saveBucket(i);
    }
}

void insert(const std::string& index, int value) {
    ensureDataDir();
    int bucket = hashIndex(index);
    loadBucket(bucket);

    auto it = std::find_if(bucketCache[bucket].begin(), bucketCache[bucket].end(),
        [&index](const Entry& e) { return e.index == index; });

    if (it != bucketCache[bucket].end()) {
        auto vit = std::lower_bound(it->values.begin(), it->values.end(), value);
        if (vit == it->values.end() || *vit != value) {
            it->values.insert(vit, value);
        }
    } else {
        bucketCache[bucket].push_back({index, {value}});
    }

    cacheDirty[bucket] = true;
}

void remove(const std::string& index, int value) {
    int bucket = hashIndex(index);
    loadBucket(bucket);

    auto it = std::find_if(bucketCache[bucket].begin(), bucketCache[bucket].end(),
        [&index](const Entry& e) { return e.index == index; });

    if (it != bucketCache[bucket].end()) {
        auto vit = std::lower_bound(it->values.begin(), it->values.end(), value);
        if (vit != it->values.end() && *vit == value) {
            it->values.erase(vit);
        }
        if (it->values.empty()) {
            bucketCache[bucket].erase(it);
        }
    }

    cacheDirty[bucket] = true;
}

void find(const std::string& index) {
    int bucket = hashIndex(index);
    loadBucket(bucket);

    auto it = std::find_if(bucketCache[bucket].begin(), bucketCache[bucket].end(),
        [&index](const Entry& e) { return e.index == index; });

    if (it == bucketCache[bucket].end() || it->values.empty()) {
        std::cout << "null" << std::endl;
    } else {
        for (size_t i = 0; i < it->values.size(); i++) {
            if (i > 0) std::cout << " ";
            std::cout << it->values[i];
        }
        std::cout << std::endl;
    }
}

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    int n;
    std::cin >> n;

    for (int i = 0; i < n; i++) {
        std::string command;
        std::cin >> command;

        if (command == "insert") {
            std::string index;
            int value;
            std::cin >> index >> value;
            insert(index, value);
        } else if (command == "delete") {
            std::string index;
            int value;
            std::cin >> index >> value;
            remove(index, value);
        } else if (command == "find") {
            std::string index;
            std::cin >> index;
            find(index);
        }
    }

    flushAllBuckets();

    return 0;
}
