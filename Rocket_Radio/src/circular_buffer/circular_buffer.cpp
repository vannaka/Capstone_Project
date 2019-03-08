#include "circular_buffer.h"

Circular_Buffer::Circular_Buffer( uint8_t size ) :
        m_buf( new uint8_t[size] ),
        m_max_size( size )
{ 
}


/**********************************************************
*   put
*       Add an item to the buffer. If the buffer is full
*       overwrites the oldest item.
**********************************************************/
void Circular_Buffer::put( uint8_t item )
{
    m_buf[m_head] = item;

    if( m_full )
    {
        m_tail = (m_tail + 1) % m_max_size;
    }

    m_head = (m_head + 1) % m_max_size;

    m_full = m_head == m_tail;
}


/**********************************************************
*   get
*       Gets the item at the head of the buffer. If the 
*       buffer is empty, zero is returned. Make sure to
*       call empty() before calling this method.
**********************************************************/
uint8_t Circular_Buffer::get()
{
    // Read data and advance the tail (we now have a free space)
    uint8_t val = m_buf[m_tail];
    m_full = false;
    m_tail = (m_tail + 1) % m_max_size;

    return val;
}


/**********************************************************
*   empty
*       Check whether the buffer is empty or not
**********************************************************/
bool Circular_Buffer::empty() const
{
    //if head and tail are equal, we are empty
    return ( !m_full && (m_head == m_tail) );
}


/**********************************************************
*   full
*       Check whether the buffer is full or not
**********************************************************/
bool Circular_Buffer::full() const
{
    //If tail is ahead the head by 1, we are full
    return m_full;
}


/**********************************************************
*   size
*       Check how many items are in the buffer.
**********************************************************/
uint8_t Circular_Buffer::size() const
{
    uint8_t size = m_max_size;

    if( !m_full )
    {
        if(m_head >= m_tail)
        {
            size = m_head - m_tail;
        }
        else
        {
            size = m_max_size + m_head - m_tail;
        }
    }

    return size;
}


/**********************************************************
*   capacity
*       Get the total size of the buffer.
**********************************************************/
uint8_t Circular_Buffer::capacity() const
{
    return m_max_size;
}


/**********************************************************
*   room
*       Check how much room is left in the buffer.
**********************************************************/
uint8_t Circular_Buffer::room() const
{
    return m_max_size - size();
}