int foo() { enum {A = 1} y = A; return y; }
int bar() { enum {A = 2}; return A; }

enum X {
    A,
    B,
    C=42,
    D,
    E
} x = E;

void main()
{
    assert_eq(foo(), 1);
    assert_eq(bar(), 2);
    assert_eq(A, 0);
    assert_eq(B, 1);
    assert_eq(C, 42);
    assert_eq(D, 43);
    assert_eq(E, 44);
    assert_eq(x, 44);
    enum X y = C-1;
    assert_eq(y, 41);
}
