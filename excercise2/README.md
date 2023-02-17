# Excercise 2

In this directory you can find the files used to produce the graphs inside the report. Inside the folders float and double the file are named by the following rules:

- Size scalability:
  - File produced by a run without the usage of numactl: "omp_policy"_"type"_"math_library"_"num_of_threads".csv
  - File produced by a run with the usage of numactl: numactl_"omp_policy"_"type"_"math_library"_"num_of_threads".csv
 
 - Core scalability:
  - File produced by a run without the usage of numactl: "size_of matrix"_"omp_policy"_"type"_"math_library"_"max_num_of_threads".csv
  - File produced by a run with the usage of numactl: "size_of matrix"numactl_"omp_policy"_"type"_"math_library"_"max_num_of_threads".csv
  
In the graphs folder I have placed all the graphs present in the report
