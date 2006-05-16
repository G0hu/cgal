#ifndef CGAL_HILBERT_SORT_3_H
#define CGAL_HILBERT_SORT_3_H

#include <CGAL/basic.h>

#include <functional>

#include <CGAL/Hilbert_sort_base.h>

CGAL_BEGIN_NAMESPACE

namespace CGALi {
    template <class K, int x, bool up> struct Hilbert_cmp_3;

    template <class K, int x>
    struct Hilbert_cmp_3<K,x,true>
        : public std::binary_function<typename K::Point_3,
                                      typename K::Point_3, bool>
    {
        typedef typename K::Point_3 Point;
        K k;
        Hilbert_cmp_3 (const K &_k = K()) : k(_k) {}
        bool operator() (const Point &p, const Point &q) const
        {
            return Hilbert_cmp_3<K,x,false> (k) (q, p);
        }
    };

    template <class K>
    struct Hilbert_cmp_3<K,0,false>
        : public std::binary_function<typename K::Point_3,
                                      typename K::Point_3, bool>
    {
        typedef typename K::Point_3 Point;
        K k;
        Hilbert_cmp_3 (const K &_k = K()) : k(_k) {}
        bool operator() (const Point &p, const Point &q) const
        {
            return k.less_x_3_object() (p, q);
        }
    };

    template <class K>
    struct Hilbert_cmp_3<K,1,false>
        : public std::binary_function<typename K::Point_3,
                                      typename K::Point_3, bool>
    {
        typedef typename K::Point_3 Point;
        K k;
        Hilbert_cmp_3 (const K &_k = K()) : k(_k) {}
        bool operator() (const Point &p, const Point &q) const
        {
            return k.less_y_3_object() (p, q);
        }
    };

    template <class K>
    struct Hilbert_cmp_3<K,2,false>
        : public std::binary_function<typename K::Point_3,
                                      typename K::Point_3, bool>
    {
        typedef typename K::Point_3 Point;
        K k;
        Hilbert_cmp_3 (const K &_k = K()) : k(_k) {}
        bool operator() (const Point &p, const Point &q) const
        {
            return k.less_z_3_object() (p, q);
        }
    };
}

template <class K>
class Hilbert_sort_3
{
public:
    typedef K Kernel;
    typedef typename Kernel::Point_3 Point;

private:
    Kernel k;

    template <int x, bool up> struct Cmp : public CGALi::Hilbert_cmp_3<Kernel,x,up>
    { Cmp (const Kernel &k) : CGALi::Hilbert_cmp_3<Kernel,x,up> (k) {} };

public:
    Hilbert_sort_3 (const Kernel &_k = Kernel()) : k(_k) {}

    template <int x, bool upx, bool upy, bool upz, class RandomAccessIterator>
    void sort (RandomAccessIterator begin, RandomAccessIterator end) const
    {
        const int y = (x + 1) % 3, z = (x + 2) % 3;
        if (end - begin <= 8) return;

        RandomAccessIterator m0 = begin, m8 = end;

        RandomAccessIterator m4 = CGALi::hilbert_split (m0, m8, Cmp< x,  upx> (k));
        RandomAccessIterator m2 = CGALi::hilbert_split (m0, m4, Cmp< y,  upy> (k));
        RandomAccessIterator m1 = CGALi::hilbert_split (m0, m2, Cmp< z,  upz> (k));
        RandomAccessIterator m3 = CGALi::hilbert_split (m2, m4, Cmp< z, !upz> (k));
        RandomAccessIterator m6 = CGALi::hilbert_split (m4, m8, Cmp< y, !upy> (k));
        RandomAccessIterator m5 = CGALi::hilbert_split (m4, m6, Cmp< z,  upz> (k));
        RandomAccessIterator m7 = CGALi::hilbert_split (m6, m8, Cmp< z, !upz> (k));

        if (end - begin <= 8*8) return;

        sort<z, upz, upx, upy> (m0, m1);
        sort<y, upy, upz, upx> (m1, m2);
        sort<y, upy, upz, upx> (m2, m3);
        sort<x, upx,!upy,!upz> (m3, m4);
        sort<x, upx,!upy,!upz> (m4, m5);
        sort<y,!upy, upz,!upx> (m5, m6);
        sort<y,!upy, upz,!upx> (m6, m7);
        sort<z,!upz,!upx, upy> (m7, m8);
    }

    template <class RandomAccessIterator>
    void operator() (RandomAccessIterator begin, RandomAccessIterator end) const
    {
        sort <0, false, false, false> (begin, end);
    }
};

CGAL_END_NAMESPACE

#endif//CGAL_HILBERT_SORT_3_H
