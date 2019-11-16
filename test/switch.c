int sw1(int x)
{
    int y = 60;
    switch (x) {
        case 0:
            y = 2;
            break;
        case 1:
            y = 3;
            break;
    }
    return y;
}

void test1()
{
    assert_eq(sw1(-1), 60);
    assert_eq(sw1(0),  2);
    assert_eq(sw1(1),  3);
    assert_eq(sw1(2),  60);
}

int sw2(int x)
{
    int y = 0;
    switch (x) {
    case 0:
        y|=1<<0;
    case 1:
        y|=1<<1;
    case 2:
        y|=1<<2;
        break;
    case 3:
        y|=1<<3;
        break;
    default:
        y|=1<<4;
    case 4:
        y|=1<<5;
    }
    return y;
}

void test2()
{
    assert_eq(sw2(-1), 1<<4|1<<5);
    assert_eq(sw2(0),  1<<0|1<<1|1<<2);
    assert_eq(sw2(1),  1<<1|1<<2);
    assert_eq(sw2(2),  1<<2);
    assert_eq(sw2(3),  1<<3);
    assert_eq(sw2(4),  1<<5);
    assert_eq(sw2(5),  1<<4|1<<5);
}

void test3()
{
    int y = 0;
    switch (42) case 42: y=1;
    assert_eq(y,1);
}

void test4()
{
    int i;
    int y = 0;
    for (i = 0; i < 10; ++i) {
        switch (i & 1) case 1: continue;
        ++y;
    }
    assert_eq(y, 5);
}

int sw5(int x)
{
    switch (x & 1) {
    case 0:
        switch (x & 2) {
        case 0: return -8;
        case 2: return -7;
        }
    case 1:
        switch (x & 2) {
        case 0: return -6;
        case 2: return -5;
        }
    }
    return 0;
}

void test5()
{
    assert_eq(sw5(-1), -5);
    assert_eq(sw5(0),  -8);
    assert_eq(sw5(1),  -6);
    assert_eq(sw5(2),  -7);
    assert_eq(sw5(3),  -5);
    assert_eq(sw5(4),  -8);
    assert_eq(sw5(5),  -6);
}

void _start()
{
    test1();
    test2();
    test3();
    test4();
    test5();
}
