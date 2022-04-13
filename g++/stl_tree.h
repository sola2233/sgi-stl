/*
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
 *
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
 */

/* NOTE: This is an internal header file, included by other STL headers.
 *   You should not attempt to use it directly.
 */

#ifndef __SGI_STL_INTERNAL_TREE_H
#define __SGI_STL_INTERNAL_TREE_H

/*

Red-black tree class, designed for use in implementing STL
associative containers (set, multiset, map, and multimap). The
insertion and deletion algorithms are based on those in Cormen,
Leiserson, and Rivest, Introduction to Algorithms (MIT Press, 1990),
except that

(1) the header cell is maintained with links not only to the root
but also to the leftmost node of the tree, to enable constant time
begin(), and to the rightmost node of the tree, to enable linear time
performance when used with the generic set algorithms (set_union,
etc.);

(2) when a node being deleted has two children its successor node is
relinked into its place, rather than copied, so that the only
iterators invalidated are those referring to the deleted node.

*/

#include <stl_algobase.h>
#include <stl_alloc.h>
#include <stl_construct.h>
#include <stl_function.h>

__STL_BEGIN_NAMESPACE 

// rb-tree 节点颜色类型
typedef bool __rb_tree_color_type;
const __rb_tree_color_type __rb_tree_red = false;   // 红色为 0
const __rb_tree_color_type __rb_tree_black = true;  // 黑色为 1

// rb-tree 节点基类
struct __rb_tree_node_base
{
  typedef __rb_tree_color_type color_type;
  typedef __rb_tree_node_base* base_ptr;

  color_type color; // 节点颜色，非黑即红 
  base_ptr parent;  // rb-tree 的许多操作必须知道父节点
  base_ptr left;    // 指向左节点
  base_ptr right;   // 指向右节点

  // 利用 bst 的性质找最大最小值
  static base_ptr minimum(base_ptr x)
  {
    while (x->left != 0) x = x->left;
    return x;
  }

  static base_ptr maximum(base_ptr x)
  {
    while (x->right != 0) x = x->right;
    return x;
  }
};

// rb-tree 节点类型
template <class Value>
struct __rb_tree_node : public __rb_tree_node_base
{
  typedef __rb_tree_node<Value>* link_type;
  Value value_field;    // 节点值
};

// 基层迭代器
struct __rb_tree_base_iterator
{
  typedef __rb_tree_node_base::base_ptr base_ptr;
  typedef bidirectional_iterator_tag iterator_category;
  typedef ptrdiff_t difference_type;
  base_ptr node;    // 指向 rb-tree 节点，用来与容器之间产生一个联结关系

  // 以下其实可以实现于 operator++ 内，因为再无他处会调用此函数了
  void increment()
  {
    if (node->right != 0) {       // 如果有右子节点，状况（1）
      node = node->right;         // 就向右走
      while (node->left != 0)     // 然后一直往左子树走到底
        node = node->left;        // 即是解答
    }
    else {                        // 没有右子节点，状况（2）
      base_ptr y = node->parent;  // 找出父节点
      while (node == y->right) {  // 如果现行节点本身是个右子节点
        node = y;                 // 就一直上溯，直到 “不为右子节点” 为止
        y = y->parent;
      }
      if (node->right != y)       // 若此时的右子节点不等于此时的父节点
        node = y;                 // 状况（3）此时的父节点即为解答
    }                             // 否则此时的 node 即为解答。状况（4）
    // 注意，以上判断 “若此时的右子节点不等于此时的父节点”，是为了应付一种
    // 特殊情况：我们欲寻找根节点的下一节点，而恰巧根节点无右子节点
    // 当然，以上特殊做法必须配合 rb-tree 根节点与特殊之 header 之间的特殊关系
  }

  // 以下其实可以实现于 operator-- 内，因为再无他处会调用此函数了
  void decrement()
  {
    if (node->color == __rb_tree_red &&   // 如果是红节点，且
        node->parent->parent == node)     // 父节点的父节点等于自己，
      node = node->right;                 // 状况（1）右子节点即为解答
    // 以上情况发生于 node 为 header 时（亦即 node 为 end() 时）
    // 注意，header 之右子节点即 mostright，指向整棵树的 max 节点
    else if (node->left != 0) {           // 如果有左子节点。状况（2）
      base_ptr y = node->left;            // 令 y 指向左子节点
      while (y->right != 0)               // 当 y 有右子节点时
        y = y->right;                     // 就一直往右子节点走到底
      node = y;                           // 最后即为答案
    }
    else {                                // 既非根节点，亦无左子节点
      base_ptr y = node->parent;          // 状况（3）找出父节点
      while (node == y->left) {           // 当现行节点身为左子节点
        node = y;                         // 一直交替往上走，直到现行节点
        y = y->parent;                    // 不为左子节点
      }
      node = y;                           // 此时之父节点即为答案
    }
  }
};

// rb-tree 的正规迭代器
template <class Value, class Ref, class Ptr>
struct __rb_tree_iterator : public __rb_tree_base_iterator
{
  typedef Value value_type;
  typedef Ref reference;
  typedef Ptr pointer;
  typedef __rb_tree_iterator<Value, Value&, Value*>             iterator;
  typedef __rb_tree_iterator<Value, const Value&, const Value*> const_iterator;
  typedef __rb_tree_iterator<Value, Ref, Ptr>                   self;
  typedef __rb_tree_node<Value>* link_type;

  __rb_tree_iterator() {}
  __rb_tree_iterator(link_type x) { node = x; }
  __rb_tree_iterator(const iterator& it) { node = it.node; }

  reference operator*() const { return link_type(node)->value_field; }
#ifndef __SGI_STL_NO_ARROW_OPERATOR
  pointer operator->() const { return &(operator*()); }
#endif /* __SGI_STL_NO_ARROW_OPERATOR */

  self& operator++() { increment(); return *this; }
  self operator++(int) {
    self tmp = *this;
    increment();
    return tmp;
  }
    
  self& operator--() { decrement(); return *this; }
  self operator--(int) {
    self tmp = *this;
    decrement();
    return tmp;
  }
};

inline bool operator==(const __rb_tree_base_iterator& x,
                       const __rb_tree_base_iterator& y) {
  return x.node == y.node;
}

inline bool operator!=(const __rb_tree_base_iterator& x,
                       const __rb_tree_base_iterator& y) {
  return x.node != y.node;
}

#ifndef __STL_CLASS_PARTIAL_SPECIALIZATION

inline bidirectional_iterator_tag
iterator_category(const __rb_tree_base_iterator&) {
  return bidirectional_iterator_tag();
}

inline __rb_tree_base_iterator::difference_type*
distance_type(const __rb_tree_base_iterator&) {
  return (__rb_tree_base_iterator::difference_type*) 0;
}

template <class Value, class Ref, class Ptr>
inline Value* value_type(const __rb_tree_iterator<Value, Ref, Ptr>&) {
  return (Value*) 0;
}

#endif /* __STL_CLASS_PARTIAL_SPECIALIZATION */

// 左旋操作，和右旋是对称的
inline void 
__rb_tree_rotate_left(__rb_tree_node_base* x, __rb_tree_node_base*& root)
{
  // x 为旋转点
  __rb_tree_node_base* y = x->right;  // 令 y 为旋转点的右子节点
  x->right = y->left;
  if (y->left !=0)
    y->left->parent = x;        // 别忘了回马枪设定父节点
  y->parent = x->parent;

  // 令 y 完全顶替 x 的地位（必须将 x 对其父节点的关系完全接收过来）
  if (x == root)                  // x 为根节点
    root = y;
  else if (x == x->parent->left)  // x 为其父节点的左子节点
    x->parent->left = y;
  else                            // x 为其父节点的右子节点
    x->parent->right = y;
  y->left = x;
  x->parent = y;
}

// 右旋操作，对应 p210 的（状况1），x 在这里就是祖父节点 G，y 对应的父节点 P
inline void 
__rb_tree_rotate_right(__rb_tree_node_base* x, __rb_tree_node_base*& root)
{
  // x 为旋转点
  __rb_tree_node_base* y = x->left;  // 令 y 为旋转点的左子节点
  x->left = y->right;
  if (y->right != 0)
    y->right->parent = x;        // 别忘了回马枪设定父节点
  y->parent = x->parent;

  // 令 y 完全顶替 x 的地位（必须将 x 对其父节点的关系完全接收过来）
  if (x == root)                  // x 为根节点
    root = y;
  else if (x == x->parent->right) // x 为其父节点的右子节点
    x->parent->right = y;
  else
    x->parent->left = y;          // x 为其父节点的左子节点
  y->right = x;
  x->parent = y;
}

// 全局函数
// 重新令树形平衡（改变颜色及旋转树形）
// 参数一为新增节点，参数二为 root
inline void 
__rb_tree_rebalance(__rb_tree_node_base* x, __rb_tree_node_base*& root)
{
  x->color = __rb_tree_red;     // 新节点必为红
  // 如果 x 不为 root 并且父节点为红
  while (x != root && x->parent->color == __rb_tree_red) {
    // 1.父节点为祖父节点之左子节点
    if (x->parent == x->parent->parent->left) { 
      __rb_tree_node_base* y = x->parent->parent->right;  // 令 y 为伯父节点
      // （状况3）和（状况4）这里不直接旋转，而是先改变颜色，直到根节点或（状况2）或（状况1）
      if (y && y->color == __rb_tree_red) {         // 伯父节点 y 存在，且为红
        x->parent->color = __rb_tree_black;         // 更改父节点为黑
        y->color = __rb_tree_black;                 // 更改伯父节点为黑
        x->parent->parent->color = __rb_tree_red;   // 更改祖父节点为红
        x = x->parent->parent;  // 准备继续往上层检查
      }
      else {  // 若无伯父节点，或伯父节点为黑
        if (x == x->parent->right) {  // 如果新节点为父节点之右子节点（状况2，双旋）
          x = x->parent;
          __rb_tree_rotate_left(x, root); // 第一参数为左旋点
        }
        // （状况1，单旋）或（状况2）的第二步单旋
        x->parent->color = __rb_tree_black;   // 改变颜色
        x->parent->parent->color = __rb_tree_red;
        __rb_tree_rotate_right(x->parent->parent, root);  // 第一参数为右旋点
      }
    }
    // 2.父节点为祖父节点之右子节点
    else {    
      __rb_tree_node_base* y = x->parent->parent->left; // 令 y 为伯父节点
      // （状况3）和（状况4）这里不直接旋转，而是先改变颜色，直到根节点或（状况2）或（状况1）
      if (y && y->color == __rb_tree_red) {         // 有伯父节点，且为红
        x->parent->color = __rb_tree_black;         // 更改父节点为黑
        y->color = __rb_tree_black;                 // 更改伯父节点为黑
        x->parent->parent->color = __rb_tree_red;   // 更改祖父节点为红
        x = x->parent->parent;  // 准备继续往上层检查
      }
      else {  // 无伯父节点，或伯父节点为黑
        if (x == x->parent->left) {   // 如果新节点为父节点之左子节点（状况2，双旋）
          x = x->parent;
          __rb_tree_rotate_right(x, root);      // 第一参数为右旋点
        }
        // （状况1，单旋）或（状况2）的第二步单旋
        x->parent->color = __rb_tree_black;     // 改变颜色
        x->parent->parent->color = __rb_tree_red;
        __rb_tree_rotate_left(x->parent->parent, root); // 第一参数为左旋点
      }
    }
  } // while 结束
  root->color = __rb_tree_black;    // 根节点永远为黑
}

inline __rb_tree_node_base*
__rb_tree_rebalance_for_erase(__rb_tree_node_base* z,
                              __rb_tree_node_base*& root,
                              __rb_tree_node_base*& leftmost,
                              __rb_tree_node_base*& rightmost)
{
  __rb_tree_node_base* y = z;
  __rb_tree_node_base* x = 0;
  __rb_tree_node_base* x_parent = 0;
  if (y->left == 0)             // z has at most one non-null child. y == z.
    x = y->right;               // x might be null.
  else
    if (y->right == 0)          // z has exactly one non-null child.  y == z.
      x = y->left;              // x is not null.
    else {                      // z has two non-null children.  Set y to
      y = y->right;             //   z's successor.  x might be null.
      while (y->left != 0)
        y = y->left;
      x = y->right;
    }
  if (y != z) {                 // relink y in place of z.  y is z's successor
    z->left->parent = y; 
    y->left = z->left;
    if (y != z->right) {
      x_parent = y->parent;
      if (x) x->parent = y->parent;
      y->parent->left = x;      // y must be a left child
      y->right = z->right;
      z->right->parent = y;
    }
    else
      x_parent = y;  
    if (root == z)
      root = y;
    else if (z->parent->left == z)
      z->parent->left = y;
    else 
      z->parent->right = y;
    y->parent = z->parent;
    __STD::swap(y->color, z->color);
    y = z;
    // y now points to node to be actually deleted
  }
  else {                        // y == z
    x_parent = y->parent;
    if (x) x->parent = y->parent;   
    if (root == z)
      root = x;
    else 
      if (z->parent->left == z)
        z->parent->left = x;
      else
        z->parent->right = x;
    if (leftmost == z) 
      if (z->right == 0)        // z->left must be null also
        leftmost = z->parent;
    // makes leftmost == header if z == root
      else
        leftmost = __rb_tree_node_base::minimum(x);
    if (rightmost == z)  
      if (z->left == 0)         // z->right must be null also
        rightmost = z->parent;  
    // makes rightmost == header if z == root
      else                      // x == z->left
        rightmost = __rb_tree_node_base::maximum(x);
  }
  if (y->color != __rb_tree_red) { 
    while (x != root && (x == 0 || x->color == __rb_tree_black))
      if (x == x_parent->left) {
        __rb_tree_node_base* w = x_parent->right;
        if (w->color == __rb_tree_red) {
          w->color = __rb_tree_black;
          x_parent->color = __rb_tree_red;
          __rb_tree_rotate_left(x_parent, root);
          w = x_parent->right;
        }
        if ((w->left == 0 || w->left->color == __rb_tree_black) &&
            (w->right == 0 || w->right->color == __rb_tree_black)) {
          w->color = __rb_tree_red;
          x = x_parent;
          x_parent = x_parent->parent;
        } else {
          if (w->right == 0 || w->right->color == __rb_tree_black) {
            if (w->left) w->left->color = __rb_tree_black;
            w->color = __rb_tree_red;
            __rb_tree_rotate_right(w, root);
            w = x_parent->right;
          }
          w->color = x_parent->color;
          x_parent->color = __rb_tree_black;
          if (w->right) w->right->color = __rb_tree_black;
          __rb_tree_rotate_left(x_parent, root);
          break;
        }
      } else {                  // same as above, with right <-> left.
        __rb_tree_node_base* w = x_parent->left;
        if (w->color == __rb_tree_red) {
          w->color = __rb_tree_black;
          x_parent->color = __rb_tree_red;
          __rb_tree_rotate_right(x_parent, root);
          w = x_parent->left;
        }
        if ((w->right == 0 || w->right->color == __rb_tree_black) &&
            (w->left == 0 || w->left->color == __rb_tree_black)) {
          w->color = __rb_tree_red;
          x = x_parent;
          x_parent = x_parent->parent;
        } else {
          if (w->left == 0 || w->left->color == __rb_tree_black) {
            if (w->right) w->right->color = __rb_tree_black;
            w->color = __rb_tree_red;
            __rb_tree_rotate_left(w, root);
            w = x_parent->left;
          }
          w->color = x_parent->color;
          x_parent->color = __rb_tree_black;
          if (w->left) w->left->color = __rb_tree_black;
          __rb_tree_rotate_right(x_parent, root);
          break;
        }
      }
    if (x) x->color = __rb_tree_black;
  }
  return y;
}

// rb-tree 结构定义
template <class Key, class Value, class KeyOfValue, class Compare,
          class Alloc = alloc>
class rb_tree {
protected:
  typedef void* void_pointer;
  typedef __rb_tree_node_base* base_ptr;
  typedef __rb_tree_node<Value> rb_tree_node;
  typedef simple_alloc<rb_tree_node, Alloc> rb_tree_node_allocator; // rb-tree 的空间配置器
  typedef __rb_tree_color_type color_type;
public:
  typedef Key key_type;
  typedef Value value_type;
  typedef value_type* pointer;
  typedef const value_type* const_pointer;
  typedef value_type& reference;
  typedef const value_type& const_reference;
  typedef rb_tree_node* link_type;
  typedef size_t size_type;
  typedef ptrdiff_t difference_type;
protected:
  link_type get_node() { return rb_tree_node_allocator::allocate(); }
  void put_node(link_type p) { rb_tree_node_allocator::deallocate(p); }

  // 创建一个节点
  link_type create_node(const value_type& x) {
    link_type tmp = get_node();         // 配置空间
    __STL_TRY {
      construct(&tmp->value_field, x);  // 构造内容
    }
    __STL_UNWIND(put_node(tmp));
    return tmp;
  }

  // 复制一个节点（的值和颜色）
  link_type clone_node(link_type x) {
    link_type tmp = create_node(x->value_field);
    tmp->color = x->color;
    tmp->left = 0;
    tmp->right = 0;
    return tmp;
  }

  // 销毁一个节点
  void destroy_node(link_type p) {
    destroy(&p->value_field); // 析构内容
    put_node(p);              // 释放内村
  }

protected:
  // rb-tree 只以三笔数据表现
  size_type node_count; // 追踪记录树的大小（节点数量）
  link_type header;     // 实现上一个技巧
  Compare key_compare;  // 节点间的 key 大小比较准则，是一个 function object（仿函数）

  // 以下三个函数用来方便地取得 header 的成员
  link_type& root() const { return (link_type&) header->parent; }
  link_type& leftmost() const { return (link_type&) header->left; }
  link_type& rightmost() const { return (link_type&) header->right; }

  // 以下六个函数用来方便地取得节点 x 的成员
  static link_type& left(link_type x) { return (link_type&)(x->left); }
  static link_type& right(link_type x) { return (link_type&)(x->right); }
  static link_type& parent(link_type x) { return (link_type&)(x->parent); }
  static reference value(link_type x) { return x->value_field; }
  static const Key& key(link_type x) { return KeyOfValue()(value(x)); }
  static color_type& color(link_type x) { return (color_type&)(x->color); }

  // 以下六个函数用来方便地取得节点 x 的成员
  static link_type& left(base_ptr x) { return (link_type&)(x->left); }
  static link_type& right(base_ptr x) { return (link_type&)(x->right); }
  static link_type& parent(base_ptr x) { return (link_type&)(x->parent); }
  static reference value(base_ptr x) { return ((link_type)x)->value_field; }
  static const Key& key(base_ptr x) { return KeyOfValue()(value(link_type(x)));} 
  static color_type& color(base_ptr x) { return (color_type&)(link_type(x)->color); }

  // 求取极大值和极小值。node class 有实现此功能，交给它们完成即可
  // fix 为什么不直接用 header 的成员呢
  static link_type minimum(link_type x) { 
    return (link_type)  __rb_tree_node_base::minimum(x);
  }
  static link_type maximum(link_type x) {
    return (link_type) __rb_tree_node_base::maximum(x);
  }

public:
  typedef __rb_tree_iterator<value_type, reference, pointer> iterator;
  typedef __rb_tree_iterator<value_type, const_reference, const_pointer> 
          const_iterator;

#ifdef __STL_CLASS_PARTIAL_SPECIALIZATION
  typedef reverse_iterator<const_iterator> const_reverse_iterator;
  typedef reverse_iterator<iterator> reverse_iterator;
#else /* __STL_CLASS_PARTIAL_SPECIALIZATION */
  typedef reverse_bidirectional_iterator<iterator, value_type, reference,
                                         difference_type>
          reverse_iterator; 
  typedef reverse_bidirectional_iterator<const_iterator, value_type,
                                         const_reference, difference_type>
          const_reverse_iterator;
#endif /* __STL_CLASS_PARTIAL_SPECIALIZATION */ 
private:
  iterator __insert(base_ptr x, base_ptr y, const value_type& v);
  link_type __copy(link_type x, link_type p);
  void __erase(link_type x);
  // 初始化一个空的 rb-tree
  void init() {
    header = get_node();            // 产生一个节点空间，令 header 指向它
    color(header) = __rb_tree_red;  // 令 header 为红色，用来区分 header
                                    // 和 root，在 iterator.operator-- 之中
    root() = 0;
    leftmost() = header;          // 令 header 的左子节点为自己
    rightmost() = header;         // 令 header 的右子节点为自己
  }
public:
                                // allocation/deallocation
  rb_tree(const Compare& comp = Compare())
    : node_count(0), key_compare(comp) { init(); }

  rb_tree(const rb_tree<Key, Value, KeyOfValue, Compare, Alloc>& x) 
    : node_count(0), key_compare(x.key_compare)
  { 
    header = get_node();
    color(header) = __rb_tree_red;
    if (x.root() == 0) {
      root() = 0;
      leftmost() = header;
      rightmost() = header;
    }
    else {
      __STL_TRY {
        root() = __copy(x.root(), header);
      }
      __STL_UNWIND(put_node(header));
      leftmost() = minimum(root());
      rightmost() = maximum(root());
    }
    node_count = x.node_count;
  }
  ~rb_tree() {
    clear();
    put_node(header);
  }
  rb_tree<Key, Value, KeyOfValue, Compare, Alloc>& 
  operator=(const rb_tree<Key, Value, KeyOfValue, Compare, Alloc>& x);

public:    
                                // accessors:
  Compare key_comp() const { return key_compare; }
  // rb-tree 的 begin 为最左（最小）节点处
  iterator begin() { return leftmost(); }
  // rb-tree 的 end 为 header 所指处
  const_iterator begin() const { return leftmost(); }
  iterator end() { return header; }
  const_iterator end() const { return header; }
  reverse_iterator rbegin() { return reverse_iterator(end()); }
  const_reverse_iterator rbegin() const { 
    return const_reverse_iterator(end()); 
  }
  reverse_iterator rend() { return reverse_iterator(begin()); }
  const_reverse_iterator rend() const { 
    return const_reverse_iterator(begin());
  } 
  bool empty() const { return node_count == 0; }
  size_type size() const { return node_count; }
  size_type max_size() const { return size_type(-1); }

  void swap(rb_tree<Key, Value, KeyOfValue, Compare, Alloc>& t) {
    __STD::swap(header, t.header);
    __STD::swap(node_count, t.node_count);
    __STD::swap(key_compare, t.key_compare);
  }
    
public:
                                // insert/erase
  // 将 x 插入到 rb-tree 中（保持节点值独一无二）
  pair<iterator,bool> insert_unique(const value_type& x);
  // 将 x 插入到 rb-tree 中（允许节点重复）
  iterator insert_equal(const value_type& x);

  iterator insert_unique(iterator position, const value_type& x);
  iterator insert_equal(iterator position, const value_type& x);

#ifdef __STL_MEMBER_TEMPLATES  
  template <class InputIterator>
  void insert_unique(InputIterator first, InputIterator last);
  template <class InputIterator>
  void insert_equal(InputIterator first, InputIterator last);
#else /* __STL_MEMBER_TEMPLATES */
  void insert_unique(const_iterator first, const_iterator last);
  void insert_unique(const value_type* first, const value_type* last);
  void insert_equal(const_iterator first, const_iterator last);
  void insert_equal(const value_type* first, const value_type* last);
#endif /* __STL_MEMBER_TEMPLATES */

  void erase(iterator position);
  size_type erase(const key_type& x);
  void erase(iterator first, iterator last);
  void erase(const key_type* first, const key_type* last);
  void clear() {
    if (node_count != 0) {
      __erase(root());
      leftmost() = header;
      root() = 0;
      rightmost() = header;
      node_count = 0;
    }
  }      

public:
                                // set operations:
  iterator find(const key_type& x);
  const_iterator find(const key_type& x) const;
  size_type count(const key_type& x) const;
  iterator lower_bound(const key_type& x);
  const_iterator lower_bound(const key_type& x) const;
  iterator upper_bound(const key_type& x);
  const_iterator upper_bound(const key_type& x) const;
  pair<iterator,iterator> equal_range(const key_type& x);
  pair<const_iterator, const_iterator> equal_range(const key_type& x) const;

public:
                                // Debugging.
  bool __rb_verify() const;
};

template <class Key, class Value, class KeyOfValue, class Compare, class Alloc>
inline bool operator==(const rb_tree<Key, Value, KeyOfValue, Compare, Alloc>& x, 
                       const rb_tree<Key, Value, KeyOfValue, Compare, Alloc>& y) {
  return x.size() == y.size() && equal(x.begin(), x.end(), y.begin());
}

template <class Key, class Value, class KeyOfValue, class Compare, class Alloc>
inline bool operator<(const rb_tree<Key, Value, KeyOfValue, Compare, Alloc>& x, 
                      const rb_tree<Key, Value, KeyOfValue, Compare, Alloc>& y) {
  return lexicographical_compare(x.begin(), x.end(), y.begin(), y.end());
}

#ifdef __STL_FUNCTION_TMPL_PARTIAL_ORDER

template <class Key, class Value, class KeyOfValue, class Compare, class Alloc>
inline void swap(rb_tree<Key, Value, KeyOfValue, Compare, Alloc>& x, 
                 rb_tree<Key, Value, KeyOfValue, Compare, Alloc>& y) {
  x.swap(y);
}

#endif /* __STL_FUNCTION_TMPL_PARTIAL_ORDER */


template <class Key, class Value, class KeyOfValue, class Compare, class Alloc>
rb_tree<Key, Value, KeyOfValue, Compare, Alloc>& 
rb_tree<Key, Value, KeyOfValue, Compare, Alloc>::
operator=(const rb_tree<Key, Value, KeyOfValue, Compare, Alloc>& x) {
  if (this != &x) {
                                // Note that Key may be a constant type.
    clear();
    node_count = 0;
    key_compare = x.key_compare;        
    if (x.root() == 0) {
      root() = 0;
      leftmost() = header;
      rightmost() = header;
    }
    else {
      root() = __copy(x.root(), header);
      leftmost() = minimum(root());
      rightmost() = maximum(root());
      node_count = x.node_count;
    }
  }
  return *this;
}

// 真正的插入执行程序需
// 参数 x_ 为新值插入点，参宿 y_ 为插入点之父节点，参数 v 为新值
template <class Key, class Value, class KeyOfValue, class Compare, class Alloc>
typename rb_tree<Key, Value, KeyOfValue, Compare, Alloc>::iterator
rb_tree<Key, Value, KeyOfValue, Compare, Alloc>::
__insert(base_ptr x_, base_ptr y_, const Value& v) {
  link_type x = (link_type) x_;
  link_type y = (link_type) y_;
  link_type z;

  // key_compare 是键值大小比较准则。是个 function object
  if (y == header || x != 0 || key_compare(KeyOfValue()(v), key(y))) {
    z = create_node(v);         // 产生一个新节点
    left(y) = z;                // 这使得当 y 为 header 时，leftmost() == z
    if (y == header) {
      root() = z;
      rightmost() = z;
    }
    else if (y == leftmost())   // 如果 y 是最左节点
      leftmost() = z;           // 维护 leftmost()，使它永远指向最左节点
  }
  else {
    z = create_node(v);         // 产生一个新节点
    right(y) = z;               // 令新节点成为插入点之父节点 y 的右子节点
    if (y == rightmost())
      rightmost() = z;          // 维护 rightmost()，使它永远指向最右节点
  }
  parent(z) = y;      // 设定新节点的父节点
  left(z) = 0;        // 设定新节点的左子节点
  right(z) = 0;       // 设定新节点的右子节点
                      // 新节点的颜色将在 __rb_tree_rebalance() 设定（并调整）
  __rb_tree_rebalance(z, header->parent); // 参数一为新增节点，参数二为 root
  ++node_count;       // 节点数累加
  return iterator(z); // 返回一个迭代器，指向新增节点
}

// 插入新值：节点值允许重复
// 注意，返回值是一个 rb-tree 迭代器，指向新增节点
template <class Key, class Value, class KeyOfValue, class Compare, class Alloc>
typename rb_tree<Key, Value, KeyOfValue, Compare, Alloc>::iterator
rb_tree<Key, Value, KeyOfValue, Compare, Alloc>::insert_equal(const Value& v)
{
  link_type y = header;
  link_type x = root();   // 从根节点开始
  while (x != 0) {        // 从根节点开始，往下寻找适当的插入点
    y = x;
    x = key_compare(KeyOfValue()(v), key(x)) ? left(x) : right(x);
    // 以上，遇 “大” 则往左，遇 “小于或等于” 则往右
  }
  return __insert(x, y, v);
  // 以上，x 为新值插入点，y 为插入点之父节点，v 为新值
}

// 插入新值：节点值不允许重复，若重复则插入无效
// 注意，返回值是个 pair，第一元素是个 rb-tree 迭代器，指向新增节点，
// 第二元素表示插入是否成功
template <class Key, class Value, class KeyOfValue, class Compare, class Alloc>
pair<typename rb_tree<Key, Value, KeyOfValue, Compare, Alloc>::iterator, bool>
rb_tree<Key, Value, KeyOfValue, Compare, Alloc>::insert_unique(const Value& v)
{
  link_type y = header;
  link_type x = root();     // 从根节点开始
  bool comp = true;
  while (x != 0) {          // 从根节点开始，往下寻找适当的插入点
    y = x;
    comp = key_compare(KeyOfValue()(v), key(x));  // v 键值小于目前节点之键值？
    x = comp ? left(x) : right(x);    // 遇 “大” 则往左，遇 “小于或等于” 则往右
  }
  // 离开 while 循环之后，y 所指即插入点之父节点（此时的它必为叶节点）

  iterator j = iterator(y);   // 令迭代器 j 指向插入点之父节点 y
  if (comp)   // 如果离开 while 循环时 comp 为真（表示遇 “大”，将插入于左侧）
    if (j == begin())     // 如果插入点之父节点为最左节点
      return pair<iterator,bool>(__insert(x, y, v), true);
      // 以上，x 为插入点，y 为插入点之父节点，v 为新值
    else  // 否则（插入点之父节点不为最左节点）
      --j;    // 看不懂这个含义？？？
  if (key_compare(key(j.node), KeyOfValue()(v)))
    // 新键值不与既有节点之键值重复，于是以下执行安插操作
    return pair<iterator,bool>(__insert(x, y, v), true);
    // 以上，x 为新值插入点，y 为插入点之父节点，v 为新值

  // 进行至此，表示新值一定与树中键值重复，那么就不该插入新值
  return pair<iterator,bool>(j, false);
}


template <class Key, class Val, class KeyOfValue, class Compare, class Alloc>
typename rb_tree<Key, Val, KeyOfValue, Compare, Alloc>::iterator 
rb_tree<Key, Val, KeyOfValue, Compare, Alloc>::insert_unique(iterator position,
                                                             const Val& v) {
  if (position.node == header->left) // begin()
    if (size() > 0 && key_compare(KeyOfValue()(v), key(position.node)))
      return __insert(position.node, position.node, v);
  // first argument just needs to be non-null 
    else
      return insert_unique(v).first;
  else if (position.node == header) // end()
    if (key_compare(key(rightmost()), KeyOfValue()(v)))
      return __insert(0, rightmost(), v);
    else
      return insert_unique(v).first;
  else {
    iterator before = position;
    --before;
    if (key_compare(key(before.node), KeyOfValue()(v))
        && key_compare(KeyOfValue()(v), key(position.node)))
      if (right(before.node) == 0)
        return __insert(0, before.node, v); 
      else
        return __insert(position.node, position.node, v);
    // first argument just needs to be non-null 
    else
      return insert_unique(v).first;
  }
}

template <class Key, class Val, class KeyOfValue, class Compare, class Alloc>
typename rb_tree<Key, Val, KeyOfValue, Compare, Alloc>::iterator 
rb_tree<Key, Val, KeyOfValue, Compare, Alloc>::insert_equal(iterator position,
                                                            const Val& v) {
  if (position.node == header->left) // begin()
    if (size() > 0 && key_compare(KeyOfValue()(v), key(position.node)))
      return __insert(position.node, position.node, v);
  // first argument just needs to be non-null 
    else
      return insert_equal(v);
  else if (position.node == header) // end()
    if (!key_compare(KeyOfValue()(v), key(rightmost())))
      return __insert(0, rightmost(), v);
    else
      return insert_equal(v);
  else {
    iterator before = position;
    --before;
    if (!key_compare(KeyOfValue()(v), key(before.node))
        && !key_compare(key(position.node), KeyOfValue()(v)))
      if (right(before.node) == 0)
        return __insert(0, before.node, v); 
      else
        return __insert(position.node, position.node, v);
    // first argument just needs to be non-null 
    else
      return insert_equal(v);
  }
}

#ifdef __STL_MEMBER_TEMPLATES  

template <class K, class V, class KoV, class Cmp, class Al> template<class II>
void rb_tree<K, V, KoV, Cmp, Al>::insert_equal(II first, II last) {
  for ( ; first != last; ++first)
    insert_equal(*first);
}

template <class K, class V, class KoV, class Cmp, class Al> template<class II>
void rb_tree<K, V, KoV, Cmp, Al>::insert_unique(II first, II last) {
  for ( ; first != last; ++first)
    insert_unique(*first);
}

#else /* __STL_MEMBER_TEMPLATES */

template <class K, class V, class KoV, class Cmp, class Al>
void
rb_tree<K, V, KoV, Cmp, Al>::insert_equal(const V* first, const V* last) {
  for ( ; first != last; ++first)
    insert_equal(*first);
}

template <class K, class V, class KoV, class Cmp, class Al>
void
rb_tree<K, V, KoV, Cmp, Al>::insert_equal(const_iterator first,
                                          const_iterator last) {
  for ( ; first != last; ++first)
    insert_equal(*first);
}

template <class K, class V, class KoV, class Cmp, class A>
void 
rb_tree<K, V, KoV, Cmp, A>::insert_unique(const V* first, const V* last) {
  for ( ; first != last; ++first)
    insert_unique(*first);
}

template <class K, class V, class KoV, class Cmp, class A>
void 
rb_tree<K, V, KoV, Cmp, A>::insert_unique(const_iterator first,
                                          const_iterator last) {
  for ( ; first != last; ++first)
    insert_unique(*first);
}

#endif /* __STL_MEMBER_TEMPLATES */
         
template <class Key, class Value, class KeyOfValue, class Compare, class Alloc>
inline void
rb_tree<Key, Value, KeyOfValue, Compare, Alloc>::erase(iterator position) {
  link_type y = (link_type) __rb_tree_rebalance_for_erase(position.node,
                                                          header->parent,
                                                          header->left,
                                                          header->right);
  destroy_node(y);
  --node_count;
}

template <class Key, class Value, class KeyOfValue, class Compare, class Alloc>
typename rb_tree<Key, Value, KeyOfValue, Compare, Alloc>::size_type 
rb_tree<Key, Value, KeyOfValue, Compare, Alloc>::erase(const Key& x) {
  pair<iterator,iterator> p = equal_range(x);
  size_type n = 0;
  distance(p.first, p.second, n);
  erase(p.first, p.second);
  return n;
}

template <class K, class V, class KeyOfValue, class Compare, class Alloc>
typename rb_tree<K, V, KeyOfValue, Compare, Alloc>::link_type 
rb_tree<K, V, KeyOfValue, Compare, Alloc>::__copy(link_type x, link_type p) {
                                // structural copy.  x and p must be non-null.
  link_type top = clone_node(x);
  top->parent = p;
 
  __STL_TRY {
    if (x->right)
      top->right = __copy(right(x), top);
    p = top;
    x = left(x);

    while (x != 0) {
      link_type y = clone_node(x);
      p->left = y;
      y->parent = p;
      if (x->right)
        y->right = __copy(right(x), y);
      p = y;
      x = left(x);
    }
  }
  __STL_UNWIND(__erase(top));

  return top;
}

template <class Key, class Value, class KeyOfValue, class Compare, class Alloc>
void rb_tree<Key, Value, KeyOfValue, Compare, Alloc>::__erase(link_type x) {
                                // erase without rebalancing
  while (x != 0) {
    __erase(right(x));
    link_type y = left(x);
    destroy_node(x);
    x = y;
  }
}

template <class Key, class Value, class KeyOfValue, class Compare, class Alloc>
void rb_tree<Key, Value, KeyOfValue, Compare, Alloc>::erase(iterator first, 
                                                            iterator last) {
  if (first == begin() && last == end())
    clear();
  else
    while (first != last) erase(first++);
}

template <class Key, class Value, class KeyOfValue, class Compare, class Alloc>
void rb_tree<Key, Value, KeyOfValue, Compare, Alloc>::erase(const Key* first, 
                                                            const Key* last) {
  while (first != last) erase(*first++);
}

// 寻找 rb-tree 中是否有键值为 k 的节点
template <class Key, class Value, class KeyOfValue, class Compare, class Alloc>
typename rb_tree<Key, Value, KeyOfValue, Compare, Alloc>::iterator 
rb_tree<Key, Value, KeyOfValue, Compare, Alloc>::find(const Key& k) {
  link_type y = header;        // Last node which is not less than k. 
  link_type x = root();        // Current node. 

  while (x != 0) 
    // 以下，key_compare 是节点 key 大小的比较准则，是个 function object
    if (!key_compare(key(x), k))
      // 若 x 的 key 大于（个人感觉还要加个等于，不然最后 y 指向什么？） k，就向左走
      y = x, x = left(x);
    else
      // 若 x 的 key 小于 k，就向右走
      x = right(x);

  iterator j = iterator(y);   
  return (j == end() || key_compare(k, key(j.node))) ? end() : j;
}

template <class Key, class Value, class KeyOfValue, class Compare, class Alloc>
typename rb_tree<Key, Value, KeyOfValue, Compare, Alloc>::const_iterator 
rb_tree<Key, Value, KeyOfValue, Compare, Alloc>::find(const Key& k) const {
  link_type y = header; /* Last node which is not less than k. */
  link_type x = root(); /* Current node. */

  while (x != 0) {
    if (!key_compare(key(x), k))
      y = x, x = left(x);
    else
      x = right(x);
  }
  const_iterator j = const_iterator(y);   
  return (j == end() || key_compare(k, key(j.node))) ? end() : j;
}

template <class Key, class Value, class KeyOfValue, class Compare, class Alloc>
typename rb_tree<Key, Value, KeyOfValue, Compare, Alloc>::size_type 
rb_tree<Key, Value, KeyOfValue, Compare, Alloc>::count(const Key& k) const {
  pair<const_iterator, const_iterator> p = equal_range(k);
  size_type n = 0;
  distance(p.first, p.second, n);
  return n;
}

template <class Key, class Value, class KeyOfValue, class Compare, class Alloc>
typename rb_tree<Key, Value, KeyOfValue, Compare, Alloc>::iterator 
rb_tree<Key, Value, KeyOfValue, Compare, Alloc>::lower_bound(const Key& k) {
  link_type y = header; /* Last node which is not less than k. */
  link_type x = root(); /* Current node. */

  while (x != 0) 
    if (!key_compare(key(x), k))
      y = x, x = left(x);
    else
      x = right(x);

  return iterator(y);
}

template <class Key, class Value, class KeyOfValue, class Compare, class Alloc>
typename rb_tree<Key, Value, KeyOfValue, Compare, Alloc>::const_iterator 
rb_tree<Key, Value, KeyOfValue, Compare, Alloc>::lower_bound(const Key& k) const {
  link_type y = header; /* Last node which is not less than k. */
  link_type x = root(); /* Current node. */

  while (x != 0) 
    if (!key_compare(key(x), k))
      y = x, x = left(x);
    else
      x = right(x);

  return const_iterator(y);
}

template <class Key, class Value, class KeyOfValue, class Compare, class Alloc>
typename rb_tree<Key, Value, KeyOfValue, Compare, Alloc>::iterator 
rb_tree<Key, Value, KeyOfValue, Compare, Alloc>::upper_bound(const Key& k) {
  link_type y = header; /* Last node which is greater than k. */
  link_type x = root(); /* Current node. */

   while (x != 0) 
     if (key_compare(k, key(x)))
       y = x, x = left(x);
     else
       x = right(x);

   return iterator(y);
}

template <class Key, class Value, class KeyOfValue, class Compare, class Alloc>
typename rb_tree<Key, Value, KeyOfValue, Compare, Alloc>::const_iterator 
rb_tree<Key, Value, KeyOfValue, Compare, Alloc>::upper_bound(const Key& k) const {
  link_type y = header; /* Last node which is greater than k. */
  link_type x = root(); /* Current node. */

   while (x != 0) 
     if (key_compare(k, key(x)))
       y = x, x = left(x);
     else
       x = right(x);

   return const_iterator(y);
}

template <class Key, class Value, class KeyOfValue, class Compare, class Alloc>
inline pair<typename rb_tree<Key, Value, KeyOfValue, Compare, Alloc>::iterator,
            typename rb_tree<Key, Value, KeyOfValue, Compare, Alloc>::iterator>
rb_tree<Key, Value, KeyOfValue, Compare, Alloc>::equal_range(const Key& k) {
  return pair<iterator, iterator>(lower_bound(k), upper_bound(k));
}

template <class Key, class Value, class KoV, class Compare, class Alloc>
inline pair<typename rb_tree<Key, Value, KoV, Compare, Alloc>::const_iterator,
            typename rb_tree<Key, Value, KoV, Compare, Alloc>::const_iterator>
rb_tree<Key, Value, KoV, Compare, Alloc>::equal_range(const Key& k) const {
  return pair<const_iterator,const_iterator>(lower_bound(k), upper_bound(k));
}

inline int __black_count(__rb_tree_node_base* node, __rb_tree_node_base* root)
{
  if (node == 0)
    return 0;
  else {
    int bc = node->color == __rb_tree_black ? 1 : 0;
    if (node == root)
      return bc;
    else
      return bc + __black_count(node->parent, root);
  }
}

template <class Key, class Value, class KeyOfValue, class Compare, class Alloc>
bool 
rb_tree<Key, Value, KeyOfValue, Compare, Alloc>::__rb_verify() const
{
  if (node_count == 0 || begin() == end())
    return node_count == 0 && begin() == end() &&
      header->left == header && header->right == header;
  
  int len = __black_count(leftmost(), root());
  for (const_iterator it = begin(); it != end(); ++it) {
    link_type x = (link_type) it.node;
    link_type L = left(x);
    link_type R = right(x);

    if (x->color == __rb_tree_red)
      if ((L && L->color == __rb_tree_red) ||
          (R && R->color == __rb_tree_red))
        return false;

    if (L && key_compare(key(x), key(L)))
      return false;
    if (R && key_compare(key(R), key(x)))
      return false;

    if (!L && !R && __black_count(x, root()) != len)
      return false;
  }

  if (leftmost() != __rb_tree_node_base::minimum(root()))
    return false;
  if (rightmost() != __rb_tree_node_base::maximum(root()))
    return false;

  return true;
}

__STL_END_NAMESPACE 

#endif /* __SGI_STL_INTERNAL_TREE_H */

// Local Variables:
// mode:C++
// End:
