#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstring>

const int NUM_BUCKETS = 20;
const std::string DATA_DIR = "data";

struct Entry {
    std::string index;
    std::vector<int> values;
};

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
    std::ifstream test(DATA_DIR + "/test");
    if (!test.good()) {
        std::system("mkdir -p data");
    }
}

std::vector<Entry> loadBucket(int bucket) {
    std::vector<Entry> entries;
    std::string filename = getBucketFilename(bucket);
    std::ifstream infile(filename, std::ios::binary);
    if (!infile.is_open()) {
        return entries;
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

        entries.push_back({index, values});
    }
    infile.close();
    return entries;
}

void saveBucket(int bucket, const std::vector<Entry>& entries) {
    std::string filename = getBucketFilename(bucket);
    std::ofstream outfile(filename, std::ios::binary);
    if (!outfile.is_open()) {
        return;
    }

    for (const auto& entry : entries) {
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
}

void insert(const std::string& index, int value) {
    ensureDataDir();
    int bucket = hashIndex(index);
    auto entries = loadBucket(bucket);

    auto it = std::find_if(entries.begin(), entries.end(),
        [&index](const Entry& e) { return e.index == index; });

    if (it != entries.end()) {
        auto vit = std::lower_bound(it->values.begin(), it->values.end(), value);
        if (vit == it->values.end() || *vit != value) {
            it->values.insert(vit, value);
        }
    } else {
        entries.push_back({index, {value}});
    }

    saveBucket(bucket, entries);
}

void remove(const std::string& index, int value) {
    int bucket = hashIndex(index);
    auto entries = loadBucket(bucket);

    auto it = std::find_if(entries.begin(), entries.end(),
        [&index](const Entry& e) { return e.index == index; });

    if (it != entries.end()) {
        auto vit = std::lower_bound(it->values.begin(), it->values.end(), value);
        if (vit != it->values.end() && *vit == value) {
            it->values.erase(vit);
        }
        if (it->values.empty()) {
            entries.erase(it);
        }
    }

    saveBucket(bucket, entries);
}

void find(const std::string& index) {
    int bucket = hashIndex(index);
    auto entries = loadBucket(bucket);

    auto it = std::find_if(entries.begin(), entries.end(),
        [&index](const Entry& e) { return e.index == index; });

    if (it == entries.end() || it->values.empty()) {
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

    return 0;
}
