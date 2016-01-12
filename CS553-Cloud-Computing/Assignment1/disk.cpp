//Jesi Merrick 
//CS553-01, Fall 2014
//Assignment 1

using namespace std;
#include <ctgmath>
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <random>
#include <climits>
#include <cstdio>

typedef std::chrono::duration<long, std::ratio<1,1000>> millisecs;
template <typename T>
long  time_since(std::chrono::time_point<T> time)
{
  auto diff = std::chrono::system_clock::now() - time;
  return std::chrono::duration_cast<millisecs>( diff ).count();
}

void seq_write_test(ulong iter, int blocksize)
{
  FILE * writefile;
  writefile=fopen("test.txt", "wb");
  rewind(writefile);
  for(ulong i=0;i<(iter);i++)
  {
    for(int j=0;j<blocksize;j++)
    {
      fputs("a",writefile);
    }
    fflush(writefile); //should force write to disk every time
  }
  //fclose(writefile);
}

void seq_read_test(ulong iter, int blocksize){
  FILE * readfile;
  readfile = fopen("test.txt", "rb");
  rewind(readfile);
  for(ulong i=0;i<(iter);i++)
  {
    for(int j=0;j<blocksize;j++)
    {
      int temp = fgetc(readfile);
    }
    fflush(readfile);
  }
}

void ra_write_test(ulong iter, int blocksize, vector<int> testData)
{
  FILE * writefile;
  writefile=fopen("test.txt", "rb+");
  rewind(writefile);
  for(ulong i=0;i<(iter);i++)
  {
    fseek(writefile, testData.at(i), SEEK_SET);
    for(int j=0;j<blocksize;j++)
    {
      fputs("a",writefile);
    }
    fflush(writefile); //should force write to disk every time
  }
}

void ra_read_test(ulong iter, int blocksize, vector<int> testData)
{
  FILE * readfile;
  readfile=fopen("test.txt", "rb");
  rewind(readfile);
  for(ulong i=0;i<(iter);i++)
  {
    fseek(readfile, testData.at(i), SEEK_SET);
    for(int j=0;j<blocksize;j++)
    {
      int temp = fgetc(readfile);
    }
    fflush(readfile);
  }
}

//Helper
void printResult(long resultTime, ulong iter, int size) {
  cout << resultTime << "ms" << endl;
  ulong speed = ((iter*size)/resultTime) * 1000;
  //cout << "TEST" << speed << "   " << (speed/1048576) << endl;
  cout << speed/1048576 << " MB/s" << endl;  
}

void gen_test_data(vector<int> &output, int num_ints, int max) {
  //cout << "Generating test ints" << endl;
  std::random_device rseed;
  std::mt19937 rgen(rseed()); //mersenne twister, apparently
  std::uniform_int_distribution<int> idist(0, max);
  for(int i=0; i<num_ints; i++) {
    output[i] = idist(rgen);
  }
}

int main(void)
{
  ulong iter=pow(10, 3);
  cout<<"Disk Benchmarking"<<endl;
  for (int i=0; i<3; i++)
  {
    int num_threads=pow(2,i);
    cout<<"\nTesting "<<num_threads<<" threads."<<endl;
    for (int j=0; j<3; j++)
    {
      int blocksize=pow(1000,j);
      cout << "\nBlock size " << blocksize << "B" << endl;
      
      thread threads[num_threads];

      //Sequential write
      cout << "Sequential Write" << endl;
      auto seq_write_before = std::chrono::system_clock::now();
      for(int i=0; i<num_threads; i++) {
        threads[i] = thread(seq_write_test, iter/num_threads, blocksize); 
      }
      for(int i=0; i<num_threads; i++) {
        threads[i].join();
      }
      auto seq_write_time = time_since(seq_write_before);
      long seq_write = (seq_write_time);
      printResult(seq_write, iter, blocksize);

      //Sequential Read
      cout << "Sequential Read" << endl;
      auto seq_read_before = std::chrono::system_clock::now();
      for(int i=0; i<num_threads; i++) {
        threads[i] = thread(seq_read_test, iter/num_threads, blocksize);
      }
      for(int i=0; i<num_threads; i++) {
        threads[i].join();
      }
      auto seq_read_time = time_since(seq_read_before);
      long seq_read = (seq_read_time);
      printResult(seq_read, iter, blocksize);
      
      //Random Read & Write
      int num_ints = iter/num_threads; 
      vector<int> test_ints(num_ints);
      gen_test_data(test_ints, num_ints, (iter*blocksize));

      //Random Write
      cout << "Random Write" << endl;
      auto ra_write_before = std::chrono::system_clock::now();
      for(int i=0; i<num_threads; i++) {
        threads[i] = thread(ra_write_test, iter/num_threads, blocksize, test_ints);
      }
      for(int i=0; i<num_threads; i++) {
        threads[i].join();
      }
      auto ra_write_time = time_since(ra_write_before);
      long ra_write = (ra_write_time);
      printResult(ra_write, iter, blocksize);

      //Random Read
      cout << "Random Read" << endl;
      auto ra_read_before = std::chrono::system_clock::now();
      for(int i=0; i<num_threads; i++) {
        threads[i] = thread(ra_read_test, iter/num_threads, blocksize, test_ints);
      }
      for(int i=0; i<num_threads; i++) {
        threads[i].join();
      }
      auto ra_read_time = time_since(ra_read_before);
      long ra_read = (ra_read_time);
      printResult(ra_read, iter, blocksize);
    }
  }
}
