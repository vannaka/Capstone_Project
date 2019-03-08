#include "Arduino.h"
#include "hdlc.h"

#include <stdint.h>


/******************************************************************************
 *                                 Defines
 *****************************************************************************/

#define FRAME_BOUNDARY_OCTET 0x7E
#define CONTROL_ESCAPE_OCTET 0x7D
#define INVERT_OCTET 0x20
#define CRC16_CCITT_INIT_VAL 0xFFFF

#define OUT_BUF_SIZE 126

/* 16bit low and high bytes copier */
#define low(x) ((x)&0xFF)
#define high(x) (((x) >> 8) & 0xFF)

#define lo8(x) ((x)&0xff)
#define hi8(x) ((x) >> 8)


/******************************************************************************
 *                        Local Function Declarations
 *****************************************************************************/

static uint16_t _crc_ccitt_update( uint16_t crc, uint8_t data );


/******************************************************************************
 *                          Method Definitions
 *****************************************************************************/

/**********************************************************
*   Hdlc
*       Constructor
**********************************************************/
Hdlc::Hdlc( uint16_t max_frame_length ) :
    frame_position( 0 ),
    max_frame_length( max_frame_length ),
    receive_frame_buffer( new uint8_t[max_frame_length + 1] ),
    lastest_data_buf( new uint8_t[max_frame_length + 1] ),
    frame_checksum( CRC16_CCITT_INIT_VAL ),
    escape_character( false )
    // out_buf(OUT_BUF_SIZE)
{
}


void Hdlc::set_send_hndlr( send_hdnlr_t const & send_byte_hndlr )
{
    m_send_byte_hndlr = send_byte_hndlr;
}


void Hdlc::set_rcv_hndlr( recv_hndlr_t const & recv_hndlr )
{
    m_recv_frame_hndlr = recv_hndlr;
}

// /**********************************************************
// *   new_frame_received
// *       Returns whether there is data ready to retrieve.
// **********************************************************/
// bool Hdlc::new_frame_received()
// {
//     bool tmp = new_data_ready;
//     new_data_ready = false;
//     return tmp;
// }

// /**********************************************************
// *   get_data
// *       Returns the data
// **********************************************************/
// void Hdlc::get_data( uint8_t * const data, uint8_t * const size )
// {
//     memcpy( data, lastest_data_buf, rdy_data_sz );
//     *size = rdy_data_sz;
// }


/**********************************************************
*   byte_receive
*       Function to find valid HDLC frame from incoming 
*       data
**********************************************************/
void Hdlc::byte_receive( uint8_t data )
{
    // We're either at the beging or end of a frame
    if( data == FRAME_BOUNDARY_OCTET )
    {
        // We expected to get an escaped char instead 
        //  we got the frame boundry. Discard partial
        //  frame.
        if( this->escape_character == true )
        {
            this->escape_character = false;
        }
        // Check if we're at the end of the frame and
        //  if its crc is valid.
        else if( ( this->frame_position >= 2 ) 
              && ( this->frame_checksum == ( (this->receive_frame_buffer[this->frame_position - 1] << 8 ) | ( this->receive_frame_buffer[this->frame_position - 2] & 0xff ) ) ) ) // (msb << 8 ) | (lsb & 0xff)
        {
            /* Call the user defined function and pass frame data to it */
            // (*receive_data_handler)( receive_frame_buffer[0], &receive_frame_buffer[1], this->frame_position - 3 );

            m_recv_frame_hndlr( receive_frame_buffer[0], &receive_frame_buffer[1], this->frame_position - 3 );
        }

        // Reset the frame
        this->frame_position = 0;
        this->frame_checksum = CRC16_CCITT_INIT_VAL;
        return;
    }

    // The previous char was an escape.
    //  Convert data back to unescaped version.
    if( this->escape_character )
    {
        this->escape_character = false;
        data ^= INVERT_OCTET;
    }
    // This char is an escape char
    else if( data == CONTROL_ESCAPE_OCTET )
    {
        this->escape_character = true;
        return;
    }

    // Add data to frame buffer
    receive_frame_buffer[ this->frame_position ] = data;

    // Update the crc16 for the received data
    if( this->frame_position - 2 >= 0 )
    {
        this->frame_checksum = _crc_ccitt_update( this->frame_checksum, receive_frame_buffer[this->frame_position - 2] );
    }

    this->frame_position++;

    // Throw away frame if we reach the max frame length
    if( this->frame_position == this->max_frame_length )
    {
        this->frame_position = 0;
        this->frame_checksum = CRC16_CCITT_INIT_VAL;
    }
}


/**********************************************************
*   send_byte
*       Method to send a byte.
**********************************************************/
void Hdlc::send_byte( uint8_t data )
{
    if( ( data == CONTROL_ESCAPE_OCTET ) 
     || ( data == FRAME_BOUNDARY_OCTET ) )
    {
        // (*this->send_byte_handler)( CONTROL_ESCAPE_OCTET );
        m_send_byte_hndlr( CONTROL_ESCAPE_OCTET );
        data ^= INVERT_OCTET;
    }

    // (*this->send_byte_handler)( data );
    m_send_byte_hndlr( data );
}


/**********************************************************
*   send_boundry_byte
*       Method to send a boundry byte.
**********************************************************/
void Hdlc::send_boundry_byte()
{
    // (*this->send_byte_handler)( FRAME_BOUNDARY_OCTET );
    m_send_byte_hndlr( FRAME_BOUNDARY_OCTET );
}


/**********************************************************
*   send_frame
*       Wrap given data in HDLC frame and send it out byte 
*       at a time
**********************************************************/
void Hdlc::send_frame( data_type_t data_type, uint8_t const * const buffer, uint8_t length )
{
    uint8_t data;
    uint16_t fcs = CRC16_CCITT_INIT_VAL;

    // Send first boundry byte
    this->send_boundry_byte();

    // Send the data_type byte
    fcs = _crc_ccitt_update( fcs, data_type );
    this->send_byte( data_type );

    // Send data
    for( int i = 0; i < length; i++ )
    {
        data = buffer[i];
        fcs = _crc_ccitt_update( fcs, data );

        this->send_byte( data );
    }

    // Send low crc
    data = low(fcs);
    this->send_byte( data );

    // Send high crc
    data = high(fcs);
    this->send_byte( data );

    // Send last boundry byte
    this->send_boundry_byte();
}


/**********************************************************
*   _crc_ccitt_update
*       Update the crc for the new data.
**********************************************************/
static uint16_t _crc_ccitt_update( uint16_t crc, uint8_t data )
{
    data ^= lo8(crc);
    data ^= data << 4;

    return ( ( ( (uint16_t)data << 8) | hi8(crc) ) ^ (uint8_t)(data >> 4) ^ ( (uint16_t)data << 3) );
}