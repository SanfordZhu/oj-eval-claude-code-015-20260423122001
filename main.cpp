#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstring>
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
    return DATA_DIR + "/bucket" + std::to_string(bucket) + ".txt";
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
    std::ifstream infile(filename);
    if (!infile.is_open()) {
        return result;
    }

    std::string line;
    while (std::getline(infile, line)) {
        if (line.empty()) continue;
        size_t space_pos = line.find(' ');
        if (space_pos == std::string::npos) continue;
        std::string index = line.substr(0, space_pos);
        if (index == target_index) {
            std::string values_str = line.substr(space_pos + 1);
            std::istringstream iss(values_str);
            int v;
            while (iss >> v) {
                result.push_back(v);
            }
            infile.close();
            return result;
        }
    }
    infile.close();
    return result;
}

void updateEntry(int bucket, const std::string& target_index, const std::vector<int>& new_values) {
    std::string filename = getBucketFilename(bucket);
    std::ifstream infile(filename);

    std::string temp_filename = filename + ".tmp";
    std::ofstream outfile(temp_filename);

    bool found = false;
    bool file_existed = infile.is_open();

    if (file_existed) {
        std::string line;
        while (std::getline(infile, line)) {
            if (line.empty()) continue;
            size_t space_pos = line.find(' ');
            if (space_pos == std::string::npos) {
                outfile << line << "\n";
                continue;
            }
            std::string index = line.substr(0, space_pos);
            if (index == target_index) {
                found = true;
                if (!new_values.empty()) {
                    outfile << target_index;
                    for (int v : new_values) {
                        outfile << " " << v;
                    }
                    outfile << "\n";
                }
            } else {
                outfile << line << "\n";
            }
        }
        infile.close();
    }

    if (!new_values.empty() && !found) {
        outfile << target_index;
        for (int v : new_values) {
            outfile << " " << v;
        }
        outfile << "\n";
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
