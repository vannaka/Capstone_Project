#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>

#include <TaskScheduler.h>

#include <Adafruit_MCP3008.h>

#include "hdlc/hdlc.h"

/******************************************************************************
 *                                 Defines
 *****************************************************************************/
#define SerialXbee Serial1
#define ADC_SS_PIN 10

#define MAX_HDLC_FRAME_LENGTH 64


/******************************************************************************
 *                               Global Types
 *****************************************************************************/
typedef struct __attribute__((packed))
{
    uint8_t rand_num;
    uint16_t adc_chnl_0;
} data_pkg_t;


/******************************************************************************
 *                          Function Declarations
 *****************************************************************************/
// Tasks
void data_send_task();
void data_receive_task();

// HDLC callbacks
void hdlc_frame_handler( const uint8_t *data, uint16_t length );
void send_byte( uint8_t data );


/******************************************************************************
 *                               Global Vars
 *****************************************************************************/
// Scheduler / Tasks
Scheduler scheduler;
Task t1( 100, TASK_FOREVER, data_send_task );
Task t2( 100, TASK_FOREVER, data_receive_task );

uint8_t rand_max = 255;

Adafruit_MCP3008 adc;

Hdlc hdlc( &send_byte, &hdlc_frame_handler, MAX_HDLC_FRAME_LENGTH );


/******************************************************************************
 *                                 Procedures
 *****************************************************************************/
void setup()
{
    Serial.begin( 9600 );
    SerialXbee.begin( 9600 );

    randomSeed( analogRead(0) );

    adc.begin( ADC_SS_PIN );

    // Setup cooperative scheduler
    scheduler.init();
    scheduler.addTask( t1 );
    scheduler.addTask( t2 );

    t1.enable();
    t2.enable();
}

void loop()
{
    scheduler.execute();
}


/**********************************************************
*   data_send_task
*       100ms task. Sends out data to ground
*       station.
**********************************************************/
void data_send_task()
{
    data_pkg_t data_pkg;

    data_pkg.rand_num = (uint8_t)random( rand_max );
    data_pkg.adc_chnl_0 = adc.readADC( 0 );

    hdlc.send_frame( (uint8_t *)&data_pkg, sizeof(data_pkg) );

}   /* dataTask() */


/**********************************************************
*   dataReceive
*       100ms task. Reads incoming bytes from the Xbee.
**********************************************************/
void data_receive_task()
{
    if( SerialXbee.available() > 0 )
    {
        uint8_t new_byte = (uint8_t)SerialXbee.read();
        hdlc.byte_receive( new_byte );
    }
}


/**********************************************************
*   hdlc_frame_handler
*       Called when a new HDLC frame has been received.
**********************************************************/
void hdlc_frame_handler( const uint8_t *data, uint16_t length )
{
    rand_max = *data;
}


/**********************************************************
*   send_character
*       HDLC lib calls this function to send a byte.
**********************************************************/
void send_byte( uint8_t data )
{
    SerialXbee.write( (uint8_t)data );
}