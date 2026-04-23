#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <filesystem>
#include <set>

namespace fs = std::filesystem;

const std::string DATA_DIR = "data";

void ensureDataDir() {
    if (!fs::exists(DATA_DIR)) {
        fs::create_directory(DATA_DIR);
    }
}

std::string getFilename(const std::string& index) {
    return DATA_DIR + "/" + index;
}

void insert(const std::string& index, int value) {
    ensureDataDir();
    std::string filename = getFilename(index);
    std::vector<int> values;

    std::ifstream infile(filename);
    if (infile.is_open()) {
        int v;
        while (infile >> v) {
            values.push_back(v);
        }
        infile.close();
    }

    auto it = std::lower_bound(values.begin(), values.end(), value);
    if (it == values.end() || *it != value) {
        values.insert(it, value);
    }

    std::ofstream outfile(filename);
    for (int v : values) {
        outfile << v << " ";
    }
    outfile.close();
}

void remove(const std::string& index, int value) {
    std::string filename = getFilename(index);
    if (!fs::exists(filename)) {
        return;
    }

    std::vector<int> values;
    std::ifstream infile(filename);
    if (infile.is_open()) {
        int v;
        while (infile >> v) {
            values.push_back(v);
        }
        infile.close();
    }

    auto it = std::lower_bound(values.begin(), values.end(), value);
    if (it != values.end() && *it == value) {
        values.erase(it);
    }

    if (values.empty()) {
        fs::remove(filename);
    } else {
        std::ofstream outfile(filename);
        for (int v : values) {
            outfile << v << " ";
        }
        outfile.close();
    }
}

void find(const std::string& index) {
    std::string filename = getFilename(index);
    if (!fs::exists(filename)) {
        std::cout << "null" << std::endl;
        return;
    }

    std::vector<int> values;
    std::ifstream infile(filename);
    if (infile.is_open()) {
        int v;
        while (infile >> v) {
            values.push_back(v);
        }
        infile.close();
    }

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
