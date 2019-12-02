struct S {
    int a;
    int b;
};

struct S* s;

int foo(int x) { return x; }

void bug1() {
    int x = 2;
    s = (struct S*)&_EBSS;
    struct S* ss = &s[foo(x)];
    ss->a = 42;
    ss->b = 60;
    assert_eq(ss, s+2);
    assert_eq(ss-s, 2);
    assert_eq(s[2].a, 42);
    assert_eq(s[2].b, 60);
    struct S* s2 = s;
    assert_eq(++s2, s+1);
    s2++;
    assert_eq(s2, ss);
    assert_eq(--s2, s+1);
    s2--;
    assert_eq(s2, s);
}

void assign() {
    struct S s1;
    s1.a = 1;
    s1.b = 2;
    struct S s2;
    s2 = s1;
    assert_eq(s2.a, 1);
    assert_eq(s2.b, 2);
}

struct S gs;

void global() {
    assert_eq(gs.a, 0);
    assert_eq(gs.b, 0);
    gs.b = 2;
    assert_eq(gs.a, 0);
    assert_eq(gs.b, 2);
    struct S ts;
    ts = gs;
    assert_eq(ts.a, 0);
    assert_eq(ts.b, 2);
    ts.a = 3;
    ts.b = 4;
    gs = ts;
    assert_eq(gs.a, 3);
    assert_eq(gs.b, 4);
}

struct L {
    struct L* next;
    int a;
};

void list() {
    struct L l1;
    struct L l2;
    l1.next = &l2;
    l2.next = 0;
    int n = 0;
    struct L* l = &l1;
    while (l) {
        ++n;
        l = l->next;
    }
    assert_eq(n, 2);
}

struct A {
    int a;
    int b;
};

struct B {
    struct A a1;
    struct A a2;
    struct C {
        int x;
    } c;
};

void nested()
{
    struct B b;
    assert_eq(sizeof(struct B), 10);
    b.a1.a = 1;
    b.a1.b = 2;
    b.a2.a = 3;
    b.a2.b = 4;
    b.c.x  = 5;
    assert_eq(b.a1.a, 1);
    assert_eq(b.a1.b, 2);
    assert_eq(b.a2.a, 3);
    assert_eq(b.a2.b, 4);
    assert_eq(b.c.x,  5);
    assert_eq((char*)&b.a2.b - (char*)&b, 6);
    assert_eq((char*)&b.c.x - (char*)&b, 8);
}

void unnamed() {
    struct {
        struct {
            int a;
            int b;
        } x;
        struct {
            int c;
        } y;
    } un;

    un.x.a=1; un.x.b=2; un.y.c=3;
    assert_eq(un.x.a, 1);
    assert_eq(un.x.b, 2);
    assert_eq(un.y.c, 3);
    assert_eq((char*)&un.y.c - (char*)&un, 4);
    assert_eq(sizeof(un), 6);
}

void scope() {
    struct A {
        int x;
    };
    struct B {
        struct A a;
    };
    struct B b1, b2;
    b1.a.x = 42;
    b2.a = b1.a;
    assert_eq(b2.a.x, 42);
}

void twice() {
    struct T {
        int a;
    };
    struct TL {
        struct T t;
    };
    struct RS {
        struct TL* tl;
    };
    struct TL tl;
    tl.t.a = 60;
    struct RS rs;
    rs.tl = &tl;
    struct RS* rsp = &rs;
    struct T t;
    t = rsp->tl->t;
}

void swap() {
    struct S {
        int a;
        int b;
    } ss[2];
    struct S temp;
    struct S * x = &ss[0], * y = &ss[1];
    ss[0].a = 42;
    ss[1].a = 60;
    temp = *x;
    *x = *y;
    *y = temp;
    assert_eq(x->a, 60);
    assert_eq(y->a, 42);
}

void cast() {
    struct T {
        char* s;
    };
    struct A {
        int i;
    };
    struct A a;
    a.i = 42;
    struct T t;
    t.s = (char*)&a;
    assert_eq(((struct A*)t.s)->i, 42);
    struct T* tp = &t;
    assert_eq(((struct A*)tp->s)->i, 42);
}

void baz(char* b, int v, int sz) {
    while (sz--) *b++ = v;
}

void with_array() {
    struct File {
        int p;
        char b[512];
    };
    struct File* CurFile = (struct File*)&_EBSS;
    baz(CurFile->b, 0, sizeof(CurFile->b));
    CurFile->p = 100;
    baz(CurFile->b + CurFile->p, 42, 2);
    assert_eq(CurFile->b[99], 0);
    assert_eq(CurFile->b[100], 42);
    assert_eq(CurFile->b[101], 42);
    assert_eq(CurFile->b[102], 0);
    assert_eq(CurFile->p, 100);
    baz(CurFile, 0, sizeof(*CurFile));
    assert_eq(CurFile->b[99], 0);
    assert_eq(CurFile->b[100], 0);
    assert_eq(CurFile->b[101], 0);
    assert_eq(CurFile->b[102], 0);
    assert_eq(CurFile->p, 0);
}

void main() {
    bug1();
    assign();
    global();
    list();
    nested();
    unnamed();
    scope();
    twice();
    swap();
    cast();
    with_array();
}
