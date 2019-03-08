#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H

#include "stdint.h"

class Circular_Buffer
{
public:
    Circular_Buffer( uint8_t size );

    void put( uint8_t item );
    uint8_t get();
    void reset();
    bool empty() const;
    bool full() const;
    uint8_t capacity() const;
    uint8_t size() const;
    uint8_t room() const;

private:
    uint8_t *m_buf;
    uint8_t m_head = 0;
    uint8_t m_tail = 0;
    const uint8_t m_max_size;
    bool m_full = false;
};

#endif