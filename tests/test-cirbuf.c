#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cirbuf.h"
#include "unit-test.h"

void TestCirbuf_set_size_with_init(CuTest *tc)
{
    cirbuf_t cb;
    cirbuf_new(&cb, 65536u);
    CuAssertTrue(tc, 65536u == cirbuf_size(&cb));
}

void TestCirbuf_is_empty_after_init(CuTest *tc)
{
    cirbuf_t cb;
    CuAssertTrue(tc, cirbuf_new(&cb, 65536));
    CuAssertTrue(tc, cirbuf_is_empty(&cb));
}

void TestCirbuf_is_not_empty_after_offer(CuTest *tc)
{
    cirbuf_t cb;
    CuAssertTrue(tc, cirbuf_new(&cb, 65536));
    cirbuf_offer(&cb, (unsigned char *) "abcd", 4);
    CuAssertTrue(tc, !cirbuf_is_empty(&cb));
}

void TestCirbuf_is_empty_after_poll_release(CuTest *tc)
{
    cirbuf_t cb;
    CuAssertTrue(tc, cirbuf_new(&cb, 65536));
    cirbuf_offer(&cb, (unsigned char *) "abcd", 4);
    cirbuf_poll(&cb, 4);
    CuAssertTrue(tc, cirbuf_is_empty(&cb));
}

void TestCirbuf_spaceused_is_zero_after_poll_release(CuTest *tc)
{
    cirbuf_t cb;
    CuAssertTrue(tc, cirbuf_new(&cb, 65536));
    cirbuf_offer(&cb, (unsigned char *) "abcd", 4);
    CuAssertTrue(tc, 4 == cirbuf_usedspace(&cb));
    cirbuf_poll(&cb, 4);
    CuAssertTrue(tc, !cirbuf_usedspace(&cb));
}

void TestCirbuf_cant_offer_if_not_enough_space(CuTest *tc)
{
    cirbuf_t cb;
    CuAssertTrue(tc, cirbuf_new(&cb, 65536));
    CuAssertTrue(tc, !cirbuf_offer(&cb, (unsigned char *) "1000", 1 << 17));
}

void TestCirbuf_cant_offer_if_buffer_will_be_completely_full(CuTest *tc)
{
    cirbuf_t cb;
    CuAssertTrue(tc, cirbuf_new(&cb, 65536));
    CuAssertTrue(tc, !cirbuf_offer(&cb, (unsigned char *) "1000", 1 << 16));
}

void TestCirbuf_offer_and_poll(CuTest *tc)
{
    cirbuf_t cb;
    CuAssertTrue(tc, cirbuf_new(&cb, 65536));
    cirbuf_offer(&cb, (unsigned char *) "abcd", 4);
    CuAssertTrue(tc, !strncmp("abcd", (char *) cirbuf_poll(&cb, 4), 4));
}

void TestCirbuf_cant_poll_nonexistant(CuTest *tc)
{
    cirbuf_t cb;
    CuAssertTrue(tc, cirbuf_new(&cb, 65536));
    CuAssertTrue(tc, NULL == cirbuf_poll(&cb, 4));
}

void TestCirbuf_cant_poll_twice_when_released(CuTest *tc)
{
    cirbuf_t cb;
    CuAssertTrue(tc, cirbuf_new(&cb, 65536));
    cirbuf_offer(&cb, (unsigned char *) "1000", 4);
    cirbuf_poll(&cb, 4);
    cirbuf_poll(&cb, 4);
    CuAssertTrue(tc, NULL == cirbuf_poll(&cb, 4));
}

void TestCirbuf_independant_of_each_other(CuTest *tc)
{
    cirbuf_t cb;
    CuAssertTrue(tc, cirbuf_new(&cb, 65536));
    cirbuf_t cb2;
    cirbuf_new(&cb2, 65536);
    cirbuf_offer(&cb, (unsigned char *) "abcd", 4);
    cirbuf_offer(&cb2, (unsigned char *) "efgh", 4);
    CuAssertTrue(tc, !strncmp("abcd", (char *) cirbuf_poll(&cb, 4), 4));
    CuAssertTrue(tc, !strncmp("efgh", (char *) cirbuf_poll(&cb2, 4), 4));
}

void TestCirbuf_independant_of_each_other_with_no_polling(CuTest *tc)
{
    cirbuf_t cb;
    CuAssertTrue(tc, cirbuf_new(&cb, 65536));
    cirbuf_t cb2;
    cirbuf_new(&cb2, 65536);
    cirbuf_offer(&cb, (unsigned char *) "abcd", 4);
    cirbuf_offer(&cb2, (unsigned char *) "efgh", 4);
    CuAssertTrue(tc, !strncmp("abcd", (char *) cirbuf_peek(&cb), 4));
    CuAssertTrue(tc, !strncmp("efgh", (char *) cirbuf_peek(&cb2), 4));
}

void TestCirbuf_correct_init(CuTest *tc)
{
    cirbuf_t cb;
    for (int i = getpagesize(); i <= getpagesize() * 4; i += 64)
        /* passes assert if initialization is successful only when
         * 'i' is a multiple of system page size */
        CuAssertTrue(tc, cirbuf_new(&cb, i) != i % getpagesize());
}

void TestCirbuf_page_flip(CuTest *tc)
{
    cirbuf_t cb;

    // fill up a memory page
    CuAssertTrue(tc, cirbuf_new(&cb, getpagesize()));
    for (int i = 0; i < getpagesize() - 2; i += 2) {
        CuAssertTrue(tc, cirbuf_offer(&cb, (unsigned char *) "aa", 2) == 2);
    }

    // this should be one of the highest addresses in the buffer
    void *high = cirbuf_peek(&cb) + cb.tail;

    // release getpagesize() / 2 of buffer's memory
    cirbuf_poll(&cb, getpagesize() / 2);
    void *high_peek = cirbuf_peek(&cb);

    // further add getpagesize() / 4 characters to memory
    for (int i = 0; i < getpagesize() / 4; i += 2) {
        CuAssertTrue(tc, cirbuf_offer(&cb, (unsigned char *) "aa", 2) == 2);
    }

    // the tail will have crossed the boundary after adding getpagesize() / 4
    // characters this address must be lower than 'void * high'
    void *low = cirbuf_peek(&cb) + cb.tail;
    CuAssertTrue(tc, high > low);

    // after the following release, head should also cross the boundary and
    // return to a low address position
    cirbuf_poll(&cb, (getpagesize() / 2) + 100);
    void *low_peek = cirbuf_peek(&cb);
    CuAssertTrue(tc, high_peek > low_peek);
}
