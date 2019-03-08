#include "xbee.h"

Xbee::Xbee( HardwareSerial *serial ) :
    m_Serial(serial),
    m_hdlc(MAX_HDLC_FRAME_LENGTH)
{
    m_hdlc.set_send_hndlr( [serial]( uint8_t byte )
    {
        serial->write( byte );
    });

    m_hdlc.set_rcv_hndlr( [this]( data_type_t data_type, uint8_t* data, uint8_t size )
    {
        // memcpy()

        
    });
}

void Xbee::setup( uint32_t baud_rate )
{
    m_Serial->begin( baud_rate );
}

uint8_t Xbee::read()
{
    if( m_Serial->available() > 0 )
    {
        m_hdlc.byte_receive( (uint8_t)m_Serial->read() );
    }

    if( m_hdlc.new_frame_received() )
    {
        m_hdlc.get_data( m_data, &m_data_sz );
        m_new_data = true;
    }
}


void Xbee::send_data( data_type_t data_type, uint8_t const * const buffer, uint8_t size )
{
    m_hdlc.send_frame( data_type, buffer, size );
}


bool Xbee::new_frame_received()
{
    bool tmp = m_new_data;
    m_new_data = false;
    return tmp;
}


void Xbee::get_data( data_type_t *data_type, uint8_t *buffer, uint8_t *size )
{
    *data_type = m_data[0];
    buffer = &m_data[1];
    *size = m_data_sz - 1;
}