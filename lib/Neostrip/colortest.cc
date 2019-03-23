/*
 * colortest.cc: console application to test the Color class stuff.
 * Extension is .cc instead of .cpp so that the samd21 Makefile ignores it.
 */

#include <stdio.h>
#include <string.h>
#include "Color.h"

template<size_t N>
class A
{
    public:
        A(void)
        {
            memset(arr, 0, sizeof(arr));
        }

        Color get(size_t n) const
        {
            if (n >= N)
                return BLACK;
            return arr[n];
        }

        void set(size_t n, const Color& val)
        {
            if (n >= N)
                return;
            arr[n] = val;
        }

        const Color operator[](size_t n) const { return get(n); }
        Color& operator[](size_t n)
        {
            static Color dummy;
            if (n >= N)
                return dummy;
            return arr[n];
        }

    private:
        Color arr[N];
};

template<size_t N>
static void print1(const A<N>& a)
{
    for (size_t i = 0; i < N; i++)
        printf("%zu:\t%d\n", i, a.get(i).i);
}

template<size_t N>
static void print2(const A<N>& a)
{
    for (int i = 0; i < (int)N; i++)
        printf("%d:\t%d\n", i, a[i].i);
}

int main()
{
    A<10> a;
    for (int i = 0; i < 10; i++)
    {
        a[i] = i;
    }
    print2(a);

    Color c1;
    c1.i = 0x0000ff;
    a[0] = c1;
    a[1] = Color{ .i = 111 };
    a[2] = 222;
    print1(a);

    return 0;
}
