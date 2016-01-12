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

typedef std::chrono::duration<long, std::ratio<1,1000>> millisecs;
template <typename T>
long  time_since(std::chrono::time_point<T> time)
{
  auto diff = std::chrono::system_clock::now() - time;
  return std::chrono::duration_cast<millisecs>( diff ).count();
}

void int_test(long long iter, vector<int> testData)
{
  int result;
  for(long long i=0;i<iter;i++)
  {
    for(int j=0; j<testData.size()/2; j++)
    {
      result = testData.at(j) * testData.at((testData.size()/2) + j);
    }
  }
}

void float_test(long long iter, vector<float> testData)
{
  float result;
  for(long long i=0;i<iter;i++)
  {
    for(int j=0; j<testData.size()/2; j++)
    {
      result = testData.at(j) * testData.at((testData.size()/2) + j);
    }
  }
}

void gen_test_data(vector<int> &output, int num_ints) {
  //cout << "Generating test ints" << endl;
  std::random_device rseed;
  std::mt19937 rgen(rseed()); //mersenne twister, apparently
  std::uniform_int_distribution<int> idist(0, INT_MAX);
  for(int i=0; i<num_ints; i++) {
    output[i] = idist(rgen);
  }
}

void gen_test_data(vector<float> &output, int num_floats) {
  //cout << "Generating test floats" << endl;
  std::random_device rseed;
  std::mt19937 rgen(rseed()); //mersenne twister, apparently
  std::uniform_real_distribution<float> fdist(0.0, (float) pow(2,31));
  for(int i=0; i<num_floats; i++) {
    output[i] = fdist(rgen);
  }
}

int main(void)
{
  int num_ints = 1024;
  int num_floats = 1024;
  vector<float> test_floats(num_floats);
  vector<int> test_ints(num_ints);
  gen_test_data(test_ints, num_ints);
  gen_test_data(test_floats, num_floats);

  long long iter=pow(2, 17);
  cout<<"CPU Benchmarking"<<endl;
  for (int i=0; i<6; i++)
  {
    int threads=pow(2,i);
    cout<<"Testing "<<threads<<" threads."<<endl;
    thread iop_threads[threads];
    thread flop_threads[threads];

    auto iop_before = std::chrono::system_clock::now();
    for(int i=0; i<threads; i++) {
      iop_threads[i] = thread(int_test, iter/threads, test_ints);
    }
    for(int i=0; i<threads; i++) {
      iop_threads[i].join();
    }
    auto iop_time = time_since(iop_before);
    long long iops = (1000*((test_ints.size()/2) * iter)) / (iop_time);
    cout << iops << " iops" << endl;

    auto flop_before = std::chrono::system_clock::now();
    for(int i=0; i<threads; i++) {
      flop_threads[i] = thread(float_test, iter/threads, test_floats);
    }
    for(int i=0; i<threads; i++) {
      flop_threads[i].join();
    }
    auto flop_time = time_since(flop_before);
    long long flops = (1000*((test_floats.size()/2) * iter)) / (flop_time);
    cout << flops << " flops" << endl;

    //double iops=2*(iter/seconds);
    //cout<<"IOPS: "<<iops<<endl;
  }
}
