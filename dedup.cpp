#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include <sha256.h> // Let's treat this as a black box

using namespace std;
namespace fs = std::filesystem;

[[noreturn]] void print_usage_and_exit(int status = EXIT_SUCCESS) {
	cout << "Usage: dedup [DIR]..\n";
	cout << "  Detects duplicated files in the given directory(s) recursively.\n";
	exit(status);
}

vector<fs::path> gather_files(int argc, char* argv[]) {
	unordered_set<string> seen;
	vector<fs::path> dirs; // We'll treat this as a stack
	for (int i = 1; i < argc; ++i) {
		auto p = fs::canonical(fs::path{argv[i]}); // This can throw exception, which is fine
		if (seen.count(p.c_str()) != 0) continue;
		seen.insert(p.c_str());
		dirs.push_back(p);
	}
	// Walk the directories
	vector<fs::path> files;
	while (!dirs.empty()) {
		fs::path d = dirs.back();
		dirs.pop_back();
		for (auto it = fs::directory_iterator(d); it != fs::directory_iterator(); ++it) {
			// Two states we care about: 1, directory 2, regular file
			if (fs::is_directory(*it)) {
				fs::path new_dir = fs::canonical(*it);
				if (seen.count(new_dir.c_str()) != 0) continue;
				dirs.push_back(new_dir);
				seen.insert(new_dir.c_str());
				continue;
			}
			if (fs::is_regular_file(*it)) {
				files.push_back(fs::canonical(*it));
			}
		}
	}
	return files;
}

int main(int argc, char* argv[]) {
	// Check arguments
	if (argc < 2) {
		print_usage_and_exit();
	}
	for (int i = 1; i < argc; ++i) {
		fs::path p{argv[i]};
		if (!fs::is_directory(p)) {
			cerr << argv[i] << " is not a directory.\n";
			print_usage_and_exit(EXIT_FAILURE);
		}
	}
	// Gather files
	cout << "Gathering files...\n\n";
	vector<fs::path> files = gather_files(argc, argv);
	cout << "Gathered " << files.size() << " files\n\n";
	// Calculate hashes
	cout << "Calculating SHA-256 hashes..\n\n";
	unordered_map<string, vector<fs::path>> visited_files; // Key: SHA-256 hash Value: list of files
	for (auto& file : files) {
		uintmax_t size = fs::file_size(file);
		if (size == 0) {
			// std::string sha256(const std::vector<char>& input);
			vector<char> e;
			auto hash = sha256(e);
			if (visited_files.count(hash) == 0) {
				visited_files[hash] = vector{file};
				continue;
			}
			visited_files[hash].push_back(file);
			continue;
		}
		ifstream ifs(file, ios::binary);
		if (!ifs) {
			cerr << "Couldn't open file: " << file.c_str() << std::endl;
			continue; // Move on
		}
		vector<char> bytes(size);
		ifs.read(bytes.data(), size);
		auto hash = sha256(bytes);
		if (visited_files.count(hash) == 0) {
			visited_files[hash] = vector{file};
			continue;
		}
		visited_files[hash].push_back(file);
	}
	// Print results
	cout << "Duplicates:\n";
	for (auto& f : visited_files) {
		if (f.second.size() == 1) continue;
		cout << "Hash: " << f.first << "\n";
		for (size_t i = 0; i < f.second.size(); ++i) {
			cout << f.second[i] << "\n";
		}
		cout << "---\n";
	}
}
