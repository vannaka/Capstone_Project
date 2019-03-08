#ifndef XBEE_H
#define XBEE_H

#include <Arduino.h>
#include <stdint.h>

#include "hdlc/hdlc.h"


/******************************************************************************
 *                                   Defines
 *****************************************************************************/
#define MAX_HDLC_FRAME_LENGTH 64

/******************************************************************************
 *                                    Xbee
 *****************************************************************************/
class Xbee
{
public:
    Xbee( HardwareSerial *serial );

    void setup( uint32_t baud_rate );

    uint8_t read();
    bool new_frame_received();
    void get_data( data_type_t *data_type, uint8_t *buffer, uint8_t *size );
    void send_data( data_type_t data_type, uint8_t const * const buffer, uint8_t size );

private:
    HardwareSerial *m_Serial;
    Hdlc m_hdlc;

    uint8_t m_data[MAX_HDLC_FRAME_LENGTH];
    uint8_t m_data_sz;
    bool m_new_data = false;
};

#endif