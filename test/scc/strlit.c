void main() {
    const char* s0 = "abc\n\r\t\'\"\\def";
    assert_eq(s0[0], 'a');
    assert_eq(s0[1], 'b');
    assert_eq(s0[2], 'c');
    assert_eq(s0[3], '\n');
    assert_eq(s0[4], '\r');
    assert_eq(s0[5], '\t');
    assert_eq(s0[6], '\'');
    assert_eq(s0[7], '"');
    assert_eq(s0[8], '\\');
    assert_eq(s0[9], 'd');
    assert_eq(s0[10], 'e');
    assert_eq(s0[11], 'f');
    assert_eq(s0[12], 0);

    const char* s1 = "123\x00\x12\xFExyz";
    assert_eq(s1[0], '1');
    assert_eq(s1[1], '2');
    assert_eq(s1[2], '3');
    assert_eq(s1[3], 0);
    assert_eq(s1[4], 0x12);
    assert_eq(s1[5]&0xff, 0xFE);
    assert_eq(s1[6], 'x');
    assert_eq(s1[7], 'y');
    assert_eq(s1[8], 'z');
    assert_eq(s1[9], 0);

    const char* s2 =
        // Comment!
        "0"
        /* */ "1"
        "2"
#if 1
        "3"
#endif
        ;
    assert_eq(s2[0], '0');
    assert_eq(s2[1], '1');
    assert_eq(s2[2], '2');
    assert_eq(s2[3], '3');
    assert_eq(s2[4], 0);

    const char* s3 = "\0x\123\377\082\129";
    assert_eq(s3[0], 0);
    assert_eq(s3[1], 'x');
    assert_eq(s3[2], 0123);
    assert_eq(s3[3], -1);
    assert_eq(s3[4], 0);
    assert_eq(s3[5], '8');
    assert_eq(s3[6], '2');
    assert_eq(s3[7], 012);
    assert_eq(s3[8], '9');
    assert_eq(s3[9], 0);
}
