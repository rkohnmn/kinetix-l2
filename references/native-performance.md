# Workflow 9 & 11: Native C++20 Rendering & Algorithmic Performance (`kinetix L2`)

## 1. Zero UI-Thread Blocking & Cache-Aligned SoA Evaluation
- **Structure-of-Arrays Batch FSRS Evaluator (`ModalityStatesSoA`)**: Evaluates 44 FSRS-4.5 power-law retention curves (`R(t) = (1 + 19/81 * t/S)^-0.5`) contiguously in `~7.1 us`.
- **O(1) Sliding-Window Separable Box/Gaussian Blur (`VectorCanvas::apply_gaussian_blur_region`)**: Uses a running-sum accumulator (`sum += in_px - out_px`) so frosted-glass card blur is independent of kernel radius.
- **Zero-Fragmentation Arena (`PoolAllocator<LinguisticNode, 4096>`)**: Allocates and recycles curriculum nodes in contiguous 4 KB pages with zero heap leaks.
