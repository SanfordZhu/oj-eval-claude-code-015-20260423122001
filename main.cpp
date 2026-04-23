#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstring>
#include <cstdint>
#include <sys/stat.h>

const int NUM_BUCKETS = 20;
const std::string DATA_DIR = "data";

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

std::vector<int> loadValues(int bucket, const std::string& target_index) {
    std::vector<int> result;
    std::string filename = getBucketFilename(bucket);
    std::ifstream infile(filename, std::ios::binary);
    if (!infile.is_open()) {
        return result;
    }

    while (infile.peek() != EOF) {
        uint16_t index_len;
        if (!infile.read(reinterpret_cast<char*>(&index_len), sizeof(index_len))) break;
        if (index_len > 100) break;

        std::string index_str(index_len, '\0');
        if (!infile.read(&index_str[0], index_len)) break;

        uint32_t value_count;
        if (!infile.read(reinterpret_cast<char*>(&value_count), sizeof(value_count))) break;
        if (value_count > 100000) break;

        if (index_str == target_index) {
            result.resize(value_count);
            for (uint32_t i = 0; i < value_count; i++) {
                int32_t v;
                if (!infile.read(reinterpret_cast<char*>(&v), sizeof(v))) {
                    result.clear();
                    infile.close();
                    return result;
                }
                result[i] = v;
            }
            infile.close();
            return result;
        }

        int32_t v;
        for (uint32_t i = 0; i < value_count; i++) {
            if (!infile.read(reinterpret_cast<char*>(&v), sizeof(v))) break;
        }
    }
    infile.close();
    return result;
}

void updateEntry(int bucket, const std::string& target_index, const std::vector<int>& new_values) {
    std::string filename = getBucketFilename(bucket);
    std::ifstream infile(filename, std::ios::binary);

    std::string temp_filename = filename + ".tmp";
    std::ofstream outfile(temp_filename, std::ios::binary);

    bool found = false;
    bool file_existed = infile.is_open();

    if (file_existed) {
        while (infile.peek() != EOF) {
            uint16_t index_len;
            if (!infile.read(reinterpret_cast<char*>(&index_len), sizeof(index_len))) break;
            if (index_len > 100) break;

            std::string index_str(index_len, '\0');
            if (!infile.read(&index_str[0], index_len)) break;

            uint32_t value_count;
            if (!infile.read(reinterpret_cast<char*>(&value_count), sizeof(value_count))) break;
            if (value_count > 100000) break;

            if (index_str == target_index) {
                found = true;
                if (!new_values.empty()) {
                    uint16_t new_index_len = static_cast<uint16_t>(target_index.size());
                    outfile.write(reinterpret_cast<const char*>(&new_index_len), sizeof(new_index_len));
                    outfile.write(target_index.data(), new_index_len);

                    uint32_t new_value_count = static_cast<uint32_t>(new_values.size());
                    outfile.write(reinterpret_cast<const char*>(&new_value_count), sizeof(new_value_count));

                    for (int v : new_values) {
                        int32_t iv = v;
                        outfile.write(reinterpret_cast<const char*>(&iv), sizeof(iv));
                    }
                }
            } else {
                uint16_t out_index_len = index_len;
                outfile.write(reinterpret_cast<const char*>(&out_index_len), sizeof(out_index_len));
                outfile.write(index_str.data(), index_len);

                uint32_t out_value_count = value_count;
                outfile.write(reinterpret_cast<const char*>(&out_value_count), sizeof(out_value_count));

                int32_t v;
                for (uint32_t i = 0; i < value_count; i++) {
                    if (!infile.read(reinterpret_cast<char*>(&v), sizeof(v))) break;
                    outfile.write(reinterpret_cast<const char*>(&v), sizeof(v));
                }
            }
        }
        infile.close();
    }

    if (!new_values.empty() && !found) {
        uint16_t index_len = static_cast<uint16_t>(target_index.size());
        outfile.write(reinterpret_cast<const char*>(&index_len), sizeof(index_len));
        outfile.write(target_index.data(), index_len);

        uint32_t value_count = static_cast<uint32_t>(new_values.size());
        outfile.write(reinterpret_cast<const char*>(&value_count), sizeof(value_count));

        for (int v : new_values) {
            int32_t iv = v;
            outfile.write(reinterpret_cast<const char*>(&iv), sizeof(iv));
        }
    }

    outfile.close();

    std::remove(filename.c_str());
    std::rename(temp_filename.c_str(), filename.c_str());
}

void insert(const std::string& index, int value) {
    ensureDataDir();
    int bucket = hashIndex(index);
    auto values = loadValues(bucket, index);

    auto it = std::lower_bound(values.begin(), values.end(), value);
    if (it == values.end() || *it != value) {
        values.insert(it, value);
    }

    updateEntry(bucket, index, values);
}

void remove(const std::string& index, int value) {
    int bucket = hashIndex(index);
    auto values = loadValues(bucket, index);

    auto it = std::lower_bound(values.begin(), values.end(), value);
    if (it != values.end() && *it == value) {
        values.erase(it);
    }

    updateEntry(bucket, index, values);
}

void find(const std::string& index) {
    int bucket = hashIndex(index);
    auto values = loadValues(bucket, index);

    if (values.empty()) {
        std::cout << "null" << std::endl;
    } else {
        for (size_t i = 0; i < values.size(); i++) {
            if (i > 0) std::cout << " ";
            std::cout << values[i];
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
