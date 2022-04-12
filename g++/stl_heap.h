/*
 *
 * Copyright (c) 1994
 * Hewlett-Packard Company
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Hewlett-Packard Company makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 * Copyright (c) 1997
 * Silicon Graphics Computer Systems, Inc.
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Silicon Graphics makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 */

/* NOTE: This is an internal header file, included by other STL headers.
 *   You should not attempt to use it directly.
 */

#ifndef __SGI_STL_INTERNAL_HEAP_H
#define __SGI_STL_INTERNAL_HEAP_H

__STL_BEGIN_NAMESPACE

#if defined(__sgi) && !defined(__GNUC__) && (_MIPS_SIM != _MIPS_SIM_ABI32)
#pragma set woff 1209
#endif

// 以下这组 push_back() 不允许指定 “大小比较标准”
template <class RandomAccessIterator, class Distance, class T>
void __push_heap(RandomAccessIterator first, Distance holeIndex,
                 Distance topIndex, T value) {
  // 找出父节点
  Distance parent = (holeIndex - 1) / 2;
  while (holeIndex > topIndex && *(first + parent) < value) { // value 就是洞值，即插入到尾部的新值
    // 当尚未到达顶端，且父节点小于新值（于是不符合 heap 的次序特性）
    // 由于以上使用 operator<，可知 STL heap 是一种 max-heap
    *(first + holeIndex) = *(first + parent); // 令洞值为父值
    holeIndex = parent; // percolate up：调整洞号，向上提升至父节点
    parent = (holeIndex - 1) / 2;   // 新洞的父节点
  }    // 持续至顶端，或满足 heap 的次序特性为止
  *(first + holeIndex) = value; // 令洞值为新值，完成插入操作
}

template <class RandomAccessIterator, class Distance, class T>
inline void __push_heap_aux(RandomAccessIterator first,
                            RandomAccessIterator last, Distance*, T*) {
  __push_heap(first, Distance((last - first) - 1), Distance(0), 
              T(*(last - 1)));
  // 以上根据 implicit representation heap 的结构特性：新值必置于底部
  // 容器的最尾端，此即第一个洞号：(last - first) - 1
}

// push_heap 就是一个上滤（percolate up）操作
template <class RandomAccessIterator>
inline void push_heap(RandomAccessIterator first, RandomAccessIterator last) {
    // 注意，此函数被调用时，新元素应已经置于底部容器的最尾端
  __push_heap_aux(first, last, distance_type(first), value_type(first));
}

template <class RandomAccessIterator, class Distance, class T, class Compare>
void __push_heap(RandomAccessIterator first, Distance holeIndex,
                 Distance topIndex, T value, Compare comp) {
  Distance parent = (holeIndex - 1) / 2;
  while (holeIndex > topIndex && comp(*(first + parent), value)) {
    *(first + holeIndex) = *(first + parent);
    holeIndex = parent;
    parent = (holeIndex - 1) / 2;
  }
  *(first + holeIndex) = value;
}

template <class RandomAccessIterator, class Compare, class Distance, class T>
inline void __push_heap_aux(RandomAccessIterator first,
                            RandomAccessIterator last, Compare comp,
                            Distance*, T*) {
  __push_heap(first, Distance((last - first) - 1), Distance(0), 
              T(*(last - 1)), comp);
}

template <class RandomAccessIterator, class Compare>
inline void push_heap(RandomAccessIterator first, RandomAccessIterator last,
                      Compare comp) {
  __push_heap_aux(first, last, comp, distance_type(first), value_type(first));
}

// 以下这个 __adjust_heap() 不允许指定 “大小比较标准”
template <class RandomAccessIterator, class Distance, class T>
void __adjust_heap(RandomAccessIterator first, Distance holeIndex,
                   Distance len, T value) {
  Distance topIndex = holeIndex;
  Distance secondChild = 2 * holeIndex + 2;   // 洞节点之右子节点
  while (secondChild < len) {
    // 比较洞节点左右两个子值，然后以 secondChild 代表较大子节点
    if (*(first + secondChild) < *(first + (secondChild - 1)))
      secondChild--;
    // percolate down：令较大子值为洞值，再令洞号下移至较大子节点处
    *(first + holeIndex) = *(first + secondChild);
    holeIndex = secondChild;
    // 找出新洞节点的右子节点
    secondChild = 2 * (secondChild + 1);
  }
  if (secondChild == len) { // 没有右子节点，只有左子节点
    // percolate down：令左子值为洞值，再令洞号下移至左子节点处
    // note 这里没有比较，直接交换了左子节点和父节点，所以有可能最后不满足 heap 的次序
    *(first + holeIndex) = *(first + (secondChild - 1));
    holeIndex = secondChild - 1;
  }
  // 此时（可能）尚未满足次序特性，执行一次 percolate up 操作
  __push_heap(first, holeIndex, topIndex, value);
}

// 以下这组 __pop_heap() 不允许指定 “大小比较标准”
template <class RandomAccessIterator, class T, class Distance>
inline void __pop_heap(RandomAccessIterator first, RandomAccessIterator last,
                       RandomAccessIterator result, T value, Distance*) {
  *result = *first; // 设定尾值为首值，于是尾值即为欲求结果
                    // 可由客户端稍后再以底层容器之 pop_back() 取出尾值
  __adjust_heap(first, Distance(0), Distance(last - first), value);
  // 以上欲重新调整 heap，洞号为 0（亦即树根处），欲调整值为 value（原尾值）
}

template <class RandomAccessIterator, class T>
inline void __pop_heap_aux(RandomAccessIterator first,
                           RandomAccessIterator last, T*) {
  __pop_heap(first, last - 1, last - 1, T(*(last - 1)), distance_type(first));
  // 以上，根据 implicit representation heap 的次序特性，pop 操作的结果
  // 应为底部容器的第一个元素。因此，首先设定欲调整值为尾值，然后将首值调至
  // 尾节点（所以以上将迭代器 result 设为 last - 1）。然后重整 [first, last - 1)，
  // 使之重新成为一个合格的 heap
}

// 下滤（percolate down）操作
template <class RandomAccessIterator>
inline void pop_heap(RandomAccessIterator first, RandomAccessIterator last) {
  __pop_heap_aux(first, last, value_type(first));
}

template <class RandomAccessIterator, class Distance, class T, class Compare>
void __adjust_heap(RandomAccessIterator first, Distance holeIndex,
                   Distance len, T value, Compare comp) {
  Distance topIndex = holeIndex;
  Distance secondChild = 2 * holeIndex + 2;
  while (secondChild < len) {
    if (comp(*(first + secondChild), *(first + (secondChild - 1))))
      secondChild--;
    *(first + holeIndex) = *(first + secondChild);
    holeIndex = secondChild;
    secondChild = 2 * (secondChild + 1);
  }
  if (secondChild == len) {
    *(first + holeIndex) = *(first + (secondChild - 1));
    holeIndex = secondChild - 1;
  }
  __push_heap(first, holeIndex, topIndex, value, comp);
}

template <class RandomAccessIterator, class T, class Compare, class Distance>
inline void __pop_heap(RandomAccessIterator first, RandomAccessIterator last,
                       RandomAccessIterator result, T value, Compare comp,
                       Distance*) {
  *result = *first;
  __adjust_heap(first, Distance(0), Distance(last - first), value, comp);
}

template <class RandomAccessIterator, class T, class Compare>
inline void __pop_heap_aux(RandomAccessIterator first,
                           RandomAccessIterator last, T*, Compare comp) {
  __pop_heap(first, last - 1, last - 1, T(*(last - 1)), comp,
             distance_type(first));
}

template <class RandomAccessIterator, class Compare>
inline void pop_heap(RandomAccessIterator first, RandomAccessIterator last,
                     Compare comp) {
    __pop_heap_aux(first, last, value_type(first), comp);
}

// 以下这组 make_heap() 不允许指定 “大小比较标准”
template <class RandomAccessIterator, class T, class Distance>
void __make_heap(RandomAccessIterator first, RandomAccessIterator last, T*,
                 Distance*) {
  // 如果长度为 0 或者 1，不必重新排列
  if (last - first < 2) return;
  Distance len = last - first;
  // 找出第一个需要重排的子树头部，以 parent 标示出。由于任何叶节点都不需要执行
  // perlocate down，所以有以下计算。parent 命名不佳，名为 holeIndex 更好
  Distance parent = (len - 2)/2;
    
  while (true) {
    // 重排以 parent 为首的子树。len 是为了让 __adjust_heap() 判断操作范围
    __adjust_heap(first, parent, len, T(*(first + parent)));
    if (parent == 0) return;    // 走完根节点，就结束
    parent--;                   // （以重排之子树的）头部向前一个节点
  }
}

// 将 [first, last) 排列为一个 heap
template <class RandomAccessIterator>
inline void make_heap(RandomAccessIterator first, RandomAccessIterator last) {
  __make_heap(first, last, value_type(first), distance_type(first));
}

template <class RandomAccessIterator, class Compare, class T, class Distance>
void __make_heap(RandomAccessIterator first, RandomAccessIterator last,
                 Compare comp, T*, Distance*) {
  if (last - first < 2) return;
  Distance len = last - first;
  Distance parent = (len - 2)/2;
    
  while (true) {
    __adjust_heap(first, parent, len, T(*(first + parent)), comp);
    if (parent == 0) return;
    parent--;
  }
}

template <class RandomAccessIterator, class Compare>
inline void make_heap(RandomAccessIterator first, RandomAccessIterator last,
                      Compare comp) {
  __make_heap(first, last, comp, value_type(first), distance_type(first));
}

// 以下这个 sort_heap() 不允许指定 “大小比较标准”
template <class RandomAccessIterator>
void sort_heap(RandomAccessIterator first, RandomAccessIterator last) {
  // 以下，每执行一次 pop_heap()，极值（在 STL heap 中为极大值）被放在尾端。
  // 扣除尾端再执行一次 pop_heap()，次极值又被放在新尾端。一直下去，最后即得
  // 排序结果
  while (last - first > 1) 
    pop_heap(first, last--);  // 每执行 pop_heap() 一次，操作范围即退缩一格
}

template <class RandomAccessIterator, class Compare>
void sort_heap(RandomAccessIterator first, RandomAccessIterator last,
               Compare comp) {
  while (last - first > 1) pop_heap(first, last--, comp);
}

#if defined(__sgi) && !defined(__GNUC__) && (_MIPS_SIM != _MIPS_SIM_ABI32)
#pragma reset woff 1209
#endif

__STL_END_NAMESPACE

#endif /* __SGI_STL_INTERNAL_HEAP_H */

// Local Variables:
// mode:C++
// End:
