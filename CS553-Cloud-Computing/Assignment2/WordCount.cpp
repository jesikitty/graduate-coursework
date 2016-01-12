#include <iostream>
#include <chrono>
#include <fstream>
#include <sstream>
#include <thread>
#include <vector>
#include <unordered_map>
using namespace std;

void wc(string buffer, unordered_map<string, int> &counts) {
  stringstream input(buffer);
  char cur;
  string word;
  string tokens = "?\";<>~`!@#&&*()_+=/\\:{}[]|,.";
  while (input.good()) {
    cur = input.get();
    if (cur == ios::eofbit) {
      break;
    }
    if (isspace(cur) || tokens.find(cur) != string::npos) {
      counts[word] += 1;
      word = "";
    } else {
      word += tolower(cur);
    }
  }
  //this_thread::sleep_for(chrono::seconds(1));
}

class Splitter {
  private:
    ifstream input;
    bool done;
    int num_threads;
    thread * workers;
    vector<unordered_map<string, int>> maps;
    unordered_map<string, int> counts;
    int mem_each;

    void start_thread(int num) {
      cerr << "starting worker " << num << endl;
      char buffer[mem_each*1024];
      //cerr << "stream good: " << input.good() << endl;
      input.get(buffer, mem_each*1024, EOF);
      //cerr << "Buffer: " << string(buffer) << endl;
      maps[num] = unordered_map<string, int>();
      //cerr << "Map clean: " << maps[num].empty() << endl;
      workers[num] = thread(wc, string(buffer), ref(maps[num]));
      if (input.eof()) {
        done = true;
        cerr << "End of file" << endl;
      }
    }

    void finish(int num) {
      workers[num].join();
      cerr << "Worker " << num << " done. Results: " << endl;
      for (auto it=maps[num].begin(); it != maps[num].end(); ++it) {
        //cerr << it->first << "\t" << it->second << endl;
        counts[it->first] += it->second;
      } // merge the counts in
    }

  public:
    Splitter(string filename, int concurrency) {
      input.open(filename);
      if (!input.is_open()) {
        cerr << "Error opening file: " << filename << endl;
      }
      mem_each = 1024; // in MB
      done = false;
      num_threads = concurrency;
      workers = new thread[num_threads];
      maps = vector<unordered_map<string, int>>(num_threads, unordered_map<string, int>());

      // Initial thread creation
      for (int i=0; i<num_threads; i++) {
        start_thread(i);
      }
    }

    unordered_map<string, int> loop() {
      // Follow up and create new threads
      int iter = 0;
      while (!done) {
        finish(iter);
        start_thread(iter);
        iter++;
        if (iter == (num_threads)) {
          iter = 0;
        } // reset iterator
      }

      for (int i=0; i<num_threads; i++) {
        finish(i);
      } // clean up at the end

      return counts;
    }
};

int main(int argc, char *argv[]) {
  // split by memory
  //int num_threads = thread::hardware_concurrency();
  int num_threads = atoi(argv[1]);
  // get from command line
  cerr << "Using " << num_threads << " threads" << endl;

  unordered_map<string, int> counts;

  cerr << "Starting worker" << endl;
  Splitter * worker = new Splitter(argv[2], num_threads);
  counts = worker->loop();

  for (auto it = counts.begin(); it != counts.end(); ++it) {
    cout << it->first << "\t" << it->second << endl;
  }
  delete worker;
  return 0;
}

