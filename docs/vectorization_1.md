```shell
cmake -E make_directory build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
# cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_FLAGS="-g" -DCMAKE_CXX_FLAGS="-g" ..
cmake --build . --config Release --parallel 8
cmake --build . --target validateLab
cmake --build . --target benchmarkLab
```


```c++
// The alignment algorithm which computes the alignment of the given sequence pairs.
result_t compute_alignment(std::vector<sequence_t> const &sequences1, std::vector<sequence_t> const &sequences2) {

    result_t result{};

    for (size_t sequence_idx = 0; sequence_idx < sequences1.size(); ++sequence_idx) {
        using score_t = int16_t;
        using column_t = std::array<score_t, sequence_size_v + 1>;

        sequence_t const &sequence1 = sequences1[sequence_idx];
        sequence_t const &sequence2 = sequences2[sequence_idx];

        /*
        * Initialise score values.
        */
        score_t gap_open{-11};
        score_t gap_extension{-1};
        score_t match{6};
        score_t mismatch{-4};

        /*
        * Setup the matrix.
        * Note we can compute the entire matrix with just one column in memory,
        * since we are only interested in the last value of the last column in the
        * score matrix.
        */
        column_t score_column{};
        column_t horizontal_gap_column{};
        score_t last_vertical_gap{};

        /*
        * Initialise the first column of the matrix.
        */
        horizontal_gap_column[0] = gap_open;
        last_vertical_gap = gap_open;

        for (size_t i = 1; i < score_column.size(); ++i) {
            score_column[i] = last_vertical_gap;
            horizontal_gap_column[i] = last_vertical_gap + gap_open;
            last_vertical_gap += gap_extension;
        }

        /*
        * Compute the main recursion to fill the matrix.
        */
        for (unsigned col = 1; col <= sequence2.size(); ++col) {
            score_t last_diagonal_score = score_column[0]; // Cache last diagonal score to compute this cell.
            score_column[0] = horizontal_gap_column[0];
            last_vertical_gap = horizontal_gap_column[0] + gap_open;
            horizontal_gap_column[0] += gap_extension;

            for (unsigned row = 1; row <= sequence1.size(); ++row) {

                // Compute next score from diagonal direction with match/mismatch.
                score_t best_cell_score =
                    last_diagonal_score +
                    (sequence1[row - 1] == sequence2[col - 1] ? match : mismatch);

                // Determine best score from diagonal, vertical, or horizontal direction.
                best_cell_score = std::max(best_cell_score, last_vertical_gap);
                best_cell_score = std::max(best_cell_score, horizontal_gap_column[row]);

                // Cache next diagonal value and store optimum in score_column.
                last_diagonal_score = score_column[row];
                score_column[row] = best_cell_score;

                // Compute the next values for vertical and horizontal gap.
                best_cell_score += gap_open;
                last_vertical_gap += gap_extension;
                horizontal_gap_column[row] += gap_extension;

                // Store optimum between gap open and gap extension.
                last_vertical_gap = std::max(last_vertical_gap, best_cell_score);
                horizontal_gap_column[row] = std::max(horizontal_gap_column[row], best_cell_score);
            }
        }

        // Report the best score.
        result[sequence_idx] = score_column.back();
    }

    return result;
}
```





```shell
> cd build

# clang 分析哪些循环被向量化
> clang++ -O3 -ffast-math -march=native -g -DNDEBUG -std=gnu++17 \
-o CMakeFiles/lab.dir/solution.cpp.o \
-c /perf-ninja/labs/core_bound/vectorization_1/solution.cpp \
-Rpass=loop-vectorize \
-Rpass-missed=loop-vectorize \
-Rpass-analysis=loop-vectorize


solution.cpp:60:7: remark: loop not vectorized: value that could not be identified as reduction is used outside the loop [-Rpass-analysis=loop-vectorize]
   60 |       for (unsigned row = 1; row <= sequence1.size(); ++row) {
      |       ^
/Users/cc/Projects/perf-ninja/labs/core_bound/vectorization_1/solution.cpp:60:7: remark: loop not vectorized [-Rpass-missed=loop-vectorize]
/Users/cc/Projects/perf-ninja/labs/core_bound/vectorization_1/solution.cpp:44:5: remark: vectorized loop (vectorization width: 8, interleaved count: 4) [-Rpass=loop-vectorize]
   44 |     for (size_t i = 1; i < score_column.size(); ++i) {
      |     ^
```





```shell
$ cmake --build . --target benchmarkLab
[100%] Built target lab
2026-06-01T08:17:15+08:00
Running ./lab
Run on (12 X 2200 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x6)
  L1 Instruction 32 KiB (x6)
  L2 Unified 256 KiB (x6)
  L3 Unified 9216 KiB (x1)
Load Average: 0.70, 1.12, 1.11
***WARNING*** CPU scaling is enabled, the benchmark real time measurements may be noisy and will incur extra overhead.
------------------------------------------------------------------
Benchmark                        Time             CPU   Iterations
------------------------------------------------------------------
bench_compute_alignment    2129296 ns      2128902 ns         1328
[100%] Built target benchmarkLab
```





---



**在数据中找寻并行性，而不是在代码中**

- build the benchmark, run it and measure the baseline running time

    ```shell
    > cmake --build . --target benchmarkLab
    ```

- 第一层分析：run the top-down analysis

    ```shell
    $ perf stat --topdown -a taskset -c 0 ./lab
    2026-06-01T08:25:42+08:00
    Running ./lab
    Run on (12 X 2200 MHz CPU s)
    CPU Caches:
      L1 Data 32 KiB (x6)
      L1 Instruction 32 KiB (x6)
      L2 Unified 256 KiB (x6)
      L3 Unified 9216 KiB (x1)
    Load Average: 0.86, 1.03, 1.07
    ***WARNING*** CPU scaling is enabled, the benchmark real time measurements may be noisy and will incur extra overhead.
    ------------------------------------------------------------------
    Benchmark                        Time             CPU   Iterations
    ------------------------------------------------------------------
    bench_compute_alignment     903792 ns       896699 ns          772
    
     Performance counter stats for 'system wide':
    
     %  tma_bad_speculation %  tma_backend_bound      %  tma_retiring %  tma_frontend_bound 
                        3.0                 36.4                    46.3                   14.3 
    
           0.809000618 seconds time elapsed
    ```

    bound by the cpu back end

- 第二层分析：

    ```shell
    # $ toplev.py --core S0-C0 -l2 --no-desc -v taskset -c 0 ./lab
    $ toplev.py --core S0-C0 -l2 --no-desc taskset -c 0 ./lab
    Consider disabling nmi watchdog to minimize multiplexing
    (echo 0 | sudo tee /proc/sys/kernel/nmi_watchdog or
     echo kernel.nmi_watchdog=0 >> /etc/sysctl.conf ; sysctl -p as root)
    Will measure complete system.
    2026-06-01T08:26:27+08:00
    Running ./lab
    Run on (12 X 2200 MHz CPU s)
    CPU Caches:
      L1 Data 32 KiB (x6)
      L1 Instruction 32 KiB (x6)
      L2 Unified 256 KiB (x6)
      L3 Unified 9216 KiB (x1)
    Load Average: 0.76, 0.99, 1.06
    ***WARNING*** CPU scaling is enabled, the benchmark real time measurements may be noisy and will incur extra overhead.
    ------------------------------------------------------------------
    Benchmark                        Time             CPU   Iterations
    ------------------------------------------------------------------
    bench_compute_alignment    1037293 ns      1001417 ns          583
    # 5.01-full-perf on Intel(R) Core(TM) i7-8750H CPU @ 2.20GHz [cfl/skylake]
    C0    BE               Backend_Bound             % Slots                       31.2   [ 8.0%]
    C0    BE/Core          Backend_Bound.Core_Bound  % Slots                       18.0   [ 8.0%]<==
    C0-T0 MUX                                        %                              8.00 
    C0-T1 MUX                                        %                              8.00 
    Run toplev --describe Core_Bound^ to get more information on bottleneck
    Add --run-sample to find locations
    Add --nodes '!+Core_Bound*/3,+MUX' for breakdown.
    ```

    almost 30% of the execution slots are attributed to the core bound category

- profile

    ```shell 
    $ perf record ./lab
    2026-06-01T21:30:36+08:00
    Running ./lab
    Run on (12 X 2200 MHz CPU s)
    CPU Caches:
      L1 Data 32 KiB (x6)
      L1 Instruction 32 KiB (x6)
      L2 Unified 256 KiB (x6)
      L3 Unified 9216 KiB (x1)
    Load Average: 0.99, 1.37, 2.02
    ***WARNING*** CPU scaling is enabled, the benchmark real time measurements may be noisy and will incur extra overhead.
    ------------------------------------------------------------------
    Benchmark                        Time             CPU   Iterations
    ------------------------------------------------------------------
    bench_compute_alignment     910552 ns       910230 ns          677
    [ perf record: Woken up 1 times to write data ]
    [ perf record: Captured and wrote 0.137 MB perf.data (2960 samples) ]
    ```
    
    
    
- take a look at profile and check hotspot

    ```shell
    > perf report -n -M intel
    
    Samples: 2K of event 'cycles:P', Event count (approx.): 1596790918
    Overhead       Samples  Command  Shared Object         Symbol
      98.18%          2899  lab      lab                   [.] compute_alignment(std::vector<std::array<unsigned char, 200ul>, std::allocator<std:◆
       0.20%             6  lab      libc.so.6             [.] __memset_avx2_unaligned_erms                                                       ▒
       0.13%             4  lab      ld-linux-x86-64.so.2  [.] _dl_lookup_symbol_x                                                                ▒
       0.09%             3  lab      ld-linux-x86-64.so.2  [.] _dl_relocate_object                                                                ▒
       0.08%             3  lab      [kernel.kallsyms]     [k] native_irq_return_iret                                                             ▒
       0.07%             2  lab      [kernel.kallsyms]     [k] task_tick_fair                                                                     ▒
       0.07%             2  lab      [kernel.kallsyms]     [k] native_write_msr                                                                   ▒
       0.07%             2  lab      [kernel.kallsyms]     [k] update_load_avg                                                                    ▒
       0.05%             2  lab      [kernel.kallsyms]     [k] get_mem_cgroup_from_mm                                                             ▒
       0.03%             1  lab      [kernel.kallsyms]     [k] entry_SYSCALL_64_after_hwframe                                                     ▒
       0.03%             1  lab      [kernel.kallsyms]     [k] perf_adjust_freq_unthr_context                                                     ▒
       0.03%             1  lab      [kernel.kallsyms]     [k] __hrtimer_run_queues                                                               ▒
       0.03%             1  lab      [kernel.kallsyms]     [k] sync_regs                                                                          ▒
       0.03%             1  lab      libstdc++.so.6.0.33   [.] std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M▒
       0.03%             1  lab      [kernel.kallsyms]     [k] account_user_time                                                                  ▒
       0.03%             1  lab      [kernel.kallsyms]     [k] raw_notifier_call_chain                                                            ▒
       0.03%             1  lab      [kernel.kallsyms]     [k] update_vsyscall                                                                    ▒
       0.03%             1  lab      [kernel.kallsyms]     [k] tick_nohz_handler                                                                  ▒
       0.03%             1  lab      [kernel.kallsyms]     [k] avg_vruntime                                                                       ▒
       0.03%             1  lab      [i915]                [k] intel_uncore_fw_release_timer                                                      ▒
       0.03%             1  lab      [kernel.kallsyms]     [k] queue_work_on                                                                      ▒
       0.03%             1  lab      [kernel.kallsyms]     [k] filemap_map_pages                                                                  ▒
       0.03%             1  lab      [kernel.kallsyms]     [k] __kvmalloc_node_noprof                                                             ▒
       0.03%             1  lab      libc.so.6             [.] __GI_____strtod_l_internal                                                         ▒
       0.03%             1  lab      [kernel.kallsyms]     [k] up_read                                                                            ▒
       0.03%             1  lab      [kernel.kallsyms]     [k] fpregs_assert_state_consistent                                                     ▒
       0.03%             1  lab      [kernel.kallsyms]     [k] irq_work_tick                                                                      ▒
       0.03%             1  lab      [kernel.kallsyms]     [k] __raw_spin_lock_irqsave                                                            ▒
       0.03%             1  lab      [kernel.kallsyms]     [k] ktime_get_update_offsets_now                                                       ▒
       0.03%             1  lab      [kernel.kallsyms]     [k] mas_store_prealloc                                                                 ▒
       0.03%             1  lab      ld-linux-x86-64.so.2  [.] strcmp                                                                             ▒
       0.03%             1  lab      [kernel.kallsyms]     [k] cap_mmap_addr                                                                      ▒
       0.03%             1  lab      [kernel.kallsyms]     [k] fsnotify_open_perm_and_set_mode                                                    ▒
       0.03%             1  lab      [kernel.kallsyms]     [k] strncpy_from_user                                                                  ▒
       0.03%             1  lab      [kernel.kallsyms]     [k] kmem_cache_alloc_noprof                                                            ▒
       0.03%             1  lab      [kernel.kallsyms]     [k] zap_present_ptes.constprop.0                                                       ▒
       0.03%             1  lab      [kernel.kallsyms]     [k] d_path                                                                             ▒
       0.03%             1  lab      libc.so.6             [.] getenv                                                                             ▒
       0.02%             1  lab      libc.so.6             [.] __strncasecmp_l                                                                    ▒
       0.02%             1  lab      [kernel.kallsyms]     [k] vfs_read                                                                           ▒
       0.02%             1  lab      [kernel.kallsyms]     [k] filemap_get_entry                                                                  ▒
       0.02%             1  lab      [kernel.kallsyms]     [k] entry_SYSRETQ_unsafe_stack                                                         ▒
       0.01%             1  lab      [kernel.kallsyms]     [k] generic_fillattr                                                                   ▒
       0.01%             1  lab      ld-linux-x86-64.so.2  [.] __GI___tunables_init                                                               ▒
       0.01%             1  lab      [kernel.kallsyms]     [k] memset_orig                                                                        ▒
       0.01%             1  lab      [kernel.kallsyms]     [k] mod_memcg_lruvec_state   
    ```







---



```c++
result_t compute_alignment(std::vector<sequence_t> const &sequences1, std::vector<sequence_t> const &sequences2)
{
    result_t result{};
    
    // transpose to the sequences
    auto trSeq1 = transpose(sequences1);
    auto trSeq2 = transpose(sequences2);

	// 原来的标量修改为向量
    using score_t = simd_score_t;
    using column_t = std::array<score_t, sequence_size_v + 1>;

    /*
     * Initialise score values.
     */
    score_t gap_open{};
    gap_open.fill(-11);
    score_t gap_extension{};
    gap_extension.fill(-1);
    score_t match{};
    match.fill(6);
    score_t mismatch{};
    mismatch.fill(-4);

    /*
     * Setup the matrix.
     * Note we can compute the entire matrix with just one column in memory,
     * since we are only interested in the last value of the last column in the
     * score matrix.
     */
    column_t score_column{};
    column_t horizontal_gap_column{};
    score_t last_vertical_gap{};

    /*
     * Initialise the first column of the matrix.
     */
    horizontal_gap_column[0] = gap_open;
    last_vertical_gap = gap_open;

    for (size_t i = 1; i < score_column.size(); ++i)
    {
        for (size_t k = 0; k < sequence_count_v; ++k)
        {
            score_column[i][k] = last_vertical_gap[k];
        	horizontal_gap_column[i][k] = last_vertical_gap[k] + gap_open[k];
        	last_vertical_gap[k] += gap_extension[k];
        }
    }

    /*
     * Compute the main recursion to fill the matrix.
     */
    for (unsigned col = 1; col <= trSeq2.size(); ++col)
    {
        score_t last_diagonal_score =
            score_column[0]; // Cache last diagonal score to compute this cell.
        
        for (size_t k = 0; k < sequence_count_v; ++k)
        {
            score_column[0][k] = horizontal_gap_column[0][k];
            last_vertical_gap[k] = horizontal_gap_column[0][k] + gap_open[k];
            horizontal_gap_column[0][k] += gap_extension[k];
        }

        for (unsigned row = 1; row <= trSeq1.size(); ++row)
        {
            // Compute next score from diagonal direction with match/mismatch.
            score_t best_cell_score = last_diagonal_score;
            for (size_t k = 0; k < sequence_count_v; ++k)
        	{
                best_cell_score[k] += (trSeq1[row - 1][k] == trSeq2[col - 1][k] ? match[k] : mismatch[k]);
            }

            for (size_t k = 0; k < sequence_count_v; ++k)
        	{
                // Determine best score from diagonal, vertical, or horizontal
                // direction.
                best_cell_score[k] = std::max(best_cell_score[k], last_vertical_gap[k]);
                best_cell_score[k] = std::max(best_cell_score[k], horizontal_gap_column[row][k]);

                // Cache next diagonal value and store optimum in score_column.
                last_diagonal_score[k] = score_column[row][k];
                score_column[row][k] = best_cell_score[k];

                // Compute the next values for vertical and horizontal gap.
                best_cell_score[k] += gap_open[k];
                last_vertical_gap[k] += gap_extension[k];
                horizontal_gap_column[row][k] += gap_extension[k];

                // Store optimum between gap open and gap extension.
                last_vertical_gap[k] = std::max(last_vertical_gap[k], best_cell_score[k]);
                horizontal_gap_column[row][k] =
                    std::max(horizontal_gap_column[row][k], best_cell_score[k]);
            }
        }
    }

    // Report the best score.
    for (size_t k = 0; k < sequence_count_v; ++k)
    {
        result[k] = score_column.back()[k];
    }
    //result[sequence_idx] = score_column.back();

    return result;
}
```



```shell
$ cmake --build . --target benchmarkLab
[ 25%] Building CXX object CMakeFiles/lab.dir/solution.cpp.o
[ 50%] Linking CXX executable lab
[100%] Built target lab
2026-06-01T08:18:03+08:00
Running ./lab
Run on (12 X 2200 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x6)
  L1 Instruction 32 KiB (x6)
  L2 Unified 256 KiB (x6)
  L3 Unified 9216 KiB (x1)
Load Average: 0.96, 1.12, 1.11
***WARNING*** CPU scaling is enabled, the benchmark real time measurements may be noisy and will incur extra overhead.
------------------------------------------------------------------
Benchmark                        Time             CPU   Iterations
------------------------------------------------------------------
bench_compute_alignment     878639 ns       878490 ns         3136
[100%] Built target benchmarkLab
```

