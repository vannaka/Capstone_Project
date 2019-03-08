#include "xbee.h"

Xbee::Xbee( HardwareSerial *serial ) :
    m_Serial(serial),
    m_hdlc(MAX_DATA_LENGTH)
{
    m_hdlc.set_send_hndlr( [serial]( uint8_t byte )
    {
        serial->write( byte );
    });

    m_hdlc.set_rcv_hndlr( [this]( uint8_t* data, uint8_t size )
    {
        memcpy( m_data, data, size );        
        m_data_sz = size;

        m_new_data = true;
    });
}

void Xbee::setup( uint32_t baud_rate )
{
    m_Serial->begin( baud_rate );
}

void Xbee::read()
{
    if( m_Serial->available() > 0 )
    {
        m_hdlc.byte_receive( (uint8_t)m_Serial->read() );
    }
}


void Xbee::send_data( data_type_t data_type, uint8_t const * const buffer, uint8_t size )
{
    uint8_t const tmp_sz = size + sizeof(data_type_t);
    uint8_t tmp_buf[ tmp_sz ];

    memcpy( tmp_buf, &data_type, sizeof(data_type_t) );
    memcpy( &tmp_buf[sizeof(data_type_t)], buffer, size );

    m_hdlc.send_frame( tmp_buf, tmp_sz );
}


bool Xbee::new_data_received()
{
    bool tmp = m_new_data;
    m_new_data = false;
    return tmp;
}


void Xbee::get_data( data_type_t &data_type, uint8_t *buffer, uint8_t &size )
{
    data_type = m_data[0];
    size = m_data_sz - 1;

    memcpy( buffer, &m_data[1], m_data_sz - 1 );
}