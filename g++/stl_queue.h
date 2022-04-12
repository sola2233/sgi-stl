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
 *
 * Copyright (c) 1996,1997
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

#ifndef __SGI_STL_INTERNAL_QUEUE_H
#define __SGI_STL_INTERNAL_QUEUE_H

__STL_BEGIN_NAMESPACE

#ifndef __STL_LIMITED_DEFAULT_TEMPLATES
template <class T, class Sequence = deque<T> >
#else
template <class T, class Sequence>
#endif
class queue {
  // 以下的 __STL_NULL_TMPL_ARGS 会展开为 <>，见 1.9.1 节
  friend bool operator== __STL_NULL_TMPL_ARGS (const queue& x, const queue& y);
  friend bool operator< __STL_NULL_TMPL_ARGS (const queue& x, const queue& y);
public:
  typedef typename Sequence::value_type value_type;
  typedef typename Sequence::size_type size_type;
  typedef typename Sequence::reference reference;
  typedef typename Sequence::const_reference const_reference;
protected:
  Sequence c;   // 底层容器
public:
  // 以下完全利用 Sequence c 的操作，完成 queue 的操作
  bool empty() const { return c.empty(); }
  size_type size() const { return c.size(); }
  reference front() { return c.front(); }
  const_reference front() const { return c.front(); }
  reference back() { return c.back(); }
  const_reference back() const { return c.back(); }
  // deque 是两头可进出，queue 是末端进、前端出（所以先进者先出）
  void push(const value_type& x) { c.push_back(x); }
  void pop() { c.pop_front(); }
};

template <class T, class Sequence>
bool operator==(const queue<T, Sequence>& x, const queue<T, Sequence>& y) {
  return x.c == y.c;
}

template <class T, class Sequence>
bool operator<(const queue<T, Sequence>& x, const queue<T, Sequence>& y) {
  return x.c < y.c;
}

// 优先级队列结构
#ifndef __STL_LIMITED_DEFAULT_TEMPLATES
template <class T, class Sequence = vector<T>, 
          class Compare = less<typename Sequence::value_type> >
#else
template <class T, class Sequence, class Compare>
#endif
class  priority_queue {
public:
  typedef typename Sequence::value_type value_type;
  typedef typename Sequence::size_type size_type;
  typedef typename Sequence::reference reference;
  typedef typename Sequence::const_reference const_reference;
protected:
  Sequence c;     // 底层容器，默认是 vector
  Compare comp;   // 元素大小比较标准，默认是 max-heap
public:
  priority_queue() : c() {}
  explicit priority_queue(const Compare& x) :  c(), comp(x) {}

// 以下用到的 make_heap()、push_heap()、pop_heap() 都是泛型算法
// 注意，任一个构造函数都立刻于底层容器内产生一个 implicit representation heap
#ifdef __STL_MEMBER_TEMPLATES
  template <class InputIterator>
  priority_queue(InputIterator first, InputIterator last, const Compare& x)
    : c(first, last), comp(x) { make_heap(c.begin(), c.end(), comp); }
  template <class InputIterator>
  priority_queue(InputIterator first, InputIterator last) 
    : c(first, last) { make_heap(c.begin(), c.end(), comp); }
#else /* __STL_MEMBER_TEMPLATES */
  priority_queue(const value_type* first, const value_type* last, 
                 const Compare& x) : c(first, last), comp(x) {
    make_heap(c.begin(), c.end(), comp);
  }
  priority_queue(const value_type* first, const value_type* last) 
    : c(first, last) { make_heap(c.begin(), c.end(), comp); }
#endif /* __STL_MEMBER_TEMPLATES */

  bool empty() const { return c.empty(); }
  size_type size() const { return c.size(); }
  const_reference top() const { return c.front(); }
  void push(const value_type& x) {
    __STL_TRY {
      // push_heap 是泛型算法，先利用底层容器的 push_back() 将新元素
      // 推入末端，再重排 heap，见 C++ Primer p.1195
      c.push_back(x); 
      push_heap(c.begin(), c.end(), comp);  // push_heap 是泛型算法
    }
    __STL_UNWIND(c.clear());
  }
  void pop() {
    __STL_TRY {
      // push_heap 是泛型算法，从 heap 内取出一个元素。它并不是真正将元素
      // 弹出，而是重排 heap，然后再以底层容器的 pop_back() 取得被弹出的
      // 元素。见 C++ Primer p.1195
      pop_heap(c.begin(), c.end(), comp);
      c.pop_back();
    }
    __STL_UNWIND(c.clear());
  }
};

// no equality is provided

__STL_END_NAMESPACE

#endif /* __SGI_STL_INTERNAL_QUEUE_H */

// Local Variables:
// mode:C++
// End:
