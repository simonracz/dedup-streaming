#include <pthread.h>
#include <sched.h>
#include <thread>
#include <mutex>
#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <cstring>

// #include <sha256.h> // Let's treat this as a black box
#include <flo-shani.h>

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

string digest_to_str(unsigned char digest[]) {
	char buf[65];
	buf[64] = 0;
	for (unsigned i = 0; i < 32; ++i) {
		sprintf(buf + i * 2, "%02x", digest[i]);
	}
	return string(buf);
}

void worker_entry(int i, mutex& m, vector<fs::path>& files, unordered_map<string, vector<fs::path>>& results) {
	while (true) {
		cpu_set_t cpuset;
		CPU_ZERO(&cpuset);
		CPU_SET(2 * i, &cpuset);
		if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) != 0) {
			cerr << "Failed to set thread affinity.\n"; // Move on
		}
		fs::path file;
		{
			std::lock_guard<std::mutex> lock(m);
			if (files.empty()) return; // Thread exit
			file = files.back();
			files.pop_back();
		}
		uint8_t digest[32];
		memset(digest, 0, 32);
		uintmax_t size = fs::file_size(file);
		if (size == 0) {
			sha256_update_shani(nullptr, 0, digest);
			auto hash = digest_to_str(digest);
			if (results.count(hash) == 0) {
				results[hash] = vector{file};
				continue;
			}
			results[hash].push_back(file);
			continue;
		}
		ifstream ifs(file, ios::binary);
		if (!ifs) {
			cerr << "Couldn't open file: " << file.c_str() << std::endl;
			continue;
		}
		char* bytes = (char* ) aligned_alloc(32, size);
		ifs.read(bytes, size);
		sha256_update_shani(reinterpret_cast<unsigned char*>(bytes), size, digest);
		free(bytes);
		auto hash = digest_to_str(digest);
		{
			std::lock_guard<std::mutex> lock(m);
			if (results.count(hash) == 0) {
				results[hash] = vector{file};
				continue;
			}
			results[hash].push_back(file);
		}
	}
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
	mutex m;
	unordered_map<string, vector<fs::path>> visited_files;
	vector<thread> threads;
	for (int i = 0; i < 8; ++i) {
		threads.emplace_back(thread(worker_entry, i, std::ref(m), std::ref(files), std::ref(visited_files)));
	}
	for (auto& t: threads) {
		t.join();
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
