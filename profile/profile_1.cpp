#include <absl/container/flat_hash_set.h>

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <random>
#include <unordered_map>
#include <unordered_set>
#include <unordered_set_bit.hh>
#include <vector>

static inline uint64_t rdtsc() {
  unsigned int lo, hi;
  __asm__ __volatile__("rdtscp" : "=a"(lo), "=d"(hi));
  return ((uint64_t)hi << 32) | lo;
}

using namespace std;

std::vector<int64_t> ins_data;
std::vector<int64_t> search_data;
std::vector<int64_t> erase_data;

typedef ds::ul_set<64, 24> ul_set_t;

static int NUM_ELEMENTS = 3000000;

void genData() {
  std::random_device
      rd;  // Will be used to obtain a seed for the random number engine
  std::mt19937 gen(123);  // Standard mersenne_twister_engine seeded with rd()
  std::uniform_int_distribution<int64_t> distrib(1, INT64_MAX);

  std::unordered_set<int64_t> s;

  for (int i = 0; i < NUM_ELEMENTS; ++i) {
    int64_t num = distrib(gen);
    if (s.find(num) != s.end()) continue;
    ins_data.push_back(num);
    s.insert(num);
  }

  // shuffle search data
  {
    search_data = ins_data;
    std::shuffle(search_data.begin(), search_data.end(), gen);
  }

  // shuffle erase data
  {
    erase_data = ins_data;
    std::shuffle(erase_data.begin(), erase_data.end(), gen);
  }
}

std::unordered_set<int64_t> us;
int nh = 0, nm = 0, ni = 0, ne = 0;

void test_stdmap_insert() {
  for (int i = 0; i < NUM_ELEMENTS; ++i) {
    int64_t v = ins_data[i], s = search_data[i];

    us.insert(v);
    ++ni;
  }
}

void test_stdmap_search() {
  for (int64_t s : search_data) {
    bool ret = us.find(s) != us.end();
    nh += ret;
    nm += !ret;
  }
}

void test_stdmap_erase() {
  for (int i = 0; i < NUM_ELEMENTS; ++i) {
    int64_t s = erase_data[i];

    ne += us.erase(s);
  }
}

void test_stdmap_insert_search() {
  for (int i = 0; i < NUM_ELEMENTS; ++i) {
    int s = search_data[i];
    int v = ins_data[i];

    ni += us.insert(v).second;
    bool ret = us.find(s) != us.end();
    nh += ret;
    nm += !ret;
  }
}

ul_set_t* ptr_um1 = nullptr;

void test_dsmap_insert() {
  auto& um1 = *ptr_um1;

  for (int i = 0; i < NUM_ELEMENTS; ++i) {
    int64_t v = ins_data[i];

    um1.insert(v);

    ++ni;
  }
}

void test_dsmap_find() {
  auto& um1 = *ptr_um1;

  for (int64_t s : search_data) {
    bool ret = um1.find(s);
    nh += ret;
    nm += !ret;
  }
}

void test_dsmap_erase() {
  auto& um1 = *ptr_um1;

  for (int i = 0; i < NUM_ELEMENTS; ++i) {
    const int64_t s = erase_data[i];
    ne += um1.erase(s);
  }
}

void test_dsmap_insert_find() {
  auto& um1 = *ptr_um1;

  for (int i = 0; i < NUM_ELEMENTS; ++i) {
    const int64_t v = ins_data[i];
    ni += um1.insert(v);
    const int64_t s = search_data[i];

    bool ret = um1.find(s);
    nh += ret;
    nm += !ret;
  }
}

absl::flat_hash_set<int64_t>* ptrFshi;

void test_fhs_insert() {
  auto& fshi = *ptrFshi;

  for (int i = 0; i < NUM_ELEMENTS; ++i) {
    int64_t v = ins_data[i];

    fshi.insert(v);
    ++ni;
  }
}

void test_fhs_search() {
  auto& fshi = *ptrFshi;

  for (int64_t s : search_data) {
    bool ret = fshi.find(s) != fshi.end();
    nh += ret;
    nm += !ret;
  }
}

void test_fhs_erase() {
  auto& fshi = *ptrFshi;

  for (int i = 0; i < NUM_ELEMENTS; ++i) {
    int64_t s = erase_data[i];

    ne += fshi.erase(s);
  }
}

int main(int argc, const char* argv[]) {
  if (argc < 2) return -1;

  ptr_um1 = new ul_set_t();
  ptrFshi = new absl::flat_hash_set<int64_t>();
  auto& fshi = *ptrFshi;

  genData();
  std::cout << "Generated data\n";

  int opt = atoi(argv[1]);

  if (opt == 1) {
    // us.reserve(NUM_ELEMENTS);
    const int64_t t1 = rdtsc();
    test_stdmap_insert();
    const int64_t t2 = rdtsc();

    test_stdmap_search();
    const int64_t t3 = rdtsc();
    std::cout << "test_stdmap insert:" << ni << "\n";
    std::cout << "test_stdmap find:" << nh << "," << nm << "\n";
    std::cout << "insert cycles " << t2 - t1 << ", search cycles " << t3 - t2
              << "\n";
    std::cout << "bucket count " << us.bucket_count() << "\n";
  } else if (opt == 2) {
    // us.reserve(NUM_ELEMENTS);
    const int64_t t1 = rdtsc();
    test_stdmap_insert_search();
    const int64_t t2 = rdtsc();
    std::cout << "test_stdmap insert_find:" << ni << "," << nh << "," << nm
              << "\n";
    std::cout << "insert + search cycles " << t2 - t1 << "\n";
    std::cout << "bucket count " << us.bucket_count() << "\n";
  } else if (opt == 3) {
    // us.reserve(NUM_ELEMENTS);
    const int64_t t1 = rdtsc();
    test_stdmap_insert();
    const int64_t t2 = rdtsc();
    test_stdmap_search();
    const int64_t t3 = rdtsc();
    test_stdmap_erase();
    const int64_t t4 = rdtsc();
    std::cout << "test_stdmap insert:" << ni << "\n";
    std::cout << "test_stdmap find:" << nh << "," << nm << "\n";
    std::cout << "test_stdmap erase:" << ne << "\n";
    std::cout << "insert cycles " << t2 - t1 << ", search cycles " << t3 - t2
              << ", erase cycles " << t4 - t3 << "\n";
  } else if (opt == 4) {
    const int64_t t1 = rdtsc();
    test_dsmap_insert();
    const int64_t t2 = rdtsc();
    test_dsmap_find();
    const int64_t t3 = rdtsc();
    std::cout << "test_dsmap insert:" << ni << "\n";
    std::cout << "test_dsmap find:" << nh << "," << nm << "\n";
    std::cout << "insert cycles " << t2 - t1 << ", search cycles " << t3 - t2
              << "\n";
  } else if (opt == 5) {
    const int64_t t1 = rdtsc();
    test_dsmap_insert_find();
    const int64_t t2 = rdtsc();
    std::cout << "test_dsmap insert_find:" << ni << "," << nh << "," << nm
              << "\n";
    std::cout << "insert + search cycles " << t2 - t1 << "\n";
  } else if (opt == 6) {
    const int64_t t1 = rdtsc();
    test_dsmap_insert();
    const int64_t t2 = rdtsc();
    test_dsmap_find();
    const int64_t t3 = rdtsc();
    test_dsmap_erase();
    const int64_t t4 = rdtsc();
    std::cout << "test_dsmap insert:" << ni << "\n";
    std::cout << "test_dsmap find:" << nh << "," << nm << "\n";
    std::cout << "test_dsmap erase:" << ne << "\n";
    std::cout << "insert cycles " << t2 - t1 << ", search cycles " << t3 - t2
              << ", erase cycles " << t4 - t3 << "\n";
  } else if (opt == 7) {
    const int64_t t1 = rdtsc();
    test_fhs_insert();
    const int64_t t2 = rdtsc();
    test_fhs_search();
    const int64_t t3 = rdtsc();
    test_fhs_erase();
    const int64_t t4 = rdtsc();
    std::cout << "test_fhs insert:" << ni << "\n";
    std::cout << "test_fhs find:" << nh << "," << nm << "\n";
    std::cout << "test_fhs erase:" << ne << "\n";
    std::cout << "insert cycles " << t2 - t1 << ", search cycles " << t3 - t2
              << ", erase cycles " << t4 - t3 << "\n";
    std::cout << "test_fhs bucket count:" << fshi.bucket_count() << "\n";
  }
}
