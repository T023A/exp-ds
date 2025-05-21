# exp-ds
This is a repo for experimental data structures.
- Unordered Hash set - If you have a use case where you only want to insert/find/erase Int64 keys (or hashes) without iterating through all elements, the implementation in the project benchmarks faster for 3 million randomly generted Int64s compared to Abseil- flash hash set; in my tests. The repo has the source implementation for the unordered set and the the profiling code.

  

To build and compare with abseil Hash, clone abseil-cpp (https://github.com/abseil/abseil-cpp) stable version under parent
directory.



Example output -

// Invoking benchmark for flat hash set

pawan@XYZ:~/unordered_search1/git/exp-ds/build$ ./ds_profile1 7

Generated data
test_fhs insert:3000000
test_fhs find:3000000,0
test_fhs erase:3000000
insert cycles 1209704280, search cycles 165655666, erase cycles 414636578
test_fhs bucket count:4194303

 
 
 
// Invoking benchmark for hash set implementation in this project

pawan@XYZ:~/unordered_search1/git/exp-ds/build$ ./ds_profile1 6

Generated data
test_dsmap insert:3000000
test_dsmap find:3000000,0
test_dsmap erase:3000000
insert cycles 383763050, search cycles 115721384, erase cycles 331029864

