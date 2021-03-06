#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <SD.h>

#include <string>

#include <TaskScheduler.h>
#include <Adafruit_MCP3008.h>
#include <Adafruit_GPS.h>

#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>

#include <AllSensors_DLV.h>

#include "xbee/xbee.h"

/******************************************************************************
 *                                 Defines
 *****************************************************************************/
#define ADC_SS_PIN 10
#define SD_SS_PIN 4

/******************************************************************************
 *                               Global Types
 *****************************************************************************/
typedef struct __attribute__((packed))
{
    uint16_t adc_chnl_0;
    uint16_t adc_chnl_1;
    uint16_t adc_chnl_2;
    uint16_t adc_chnl_3;
    uint16_t adc_chnl_4;
    uint16_t adc_chnl_5;
    uint16_t adc_chnl_6;
    uint16_t adc_chnl_7;
    float angle_x;
    float angle_y;
    float angle_z;
    float accel_x;
    float accel_y;
    float accel_z;
    float prsur;
    float prsur_temp;
} data_pkg_t;

typedef struct __attribute__((packed))
{
    uint8_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t min;
    uint8_t sec;
    float lat;
    float lon;
    bool fix;
    uint8_t fix_qual;
    uint8_t sat_num;
} gps_data_t;

typedef uint8_t data_log_sts_t;
enum
{
    DATA_LOG_STS_START = 0,
    DATA_LOG_STS_STOP  = 1,
};


/******************************************************************************
 *                          Function Declarations
 *****************************************************************************/
// Tasks
void data_send_task();
void gps_send_task();
void data_collect_task();

//SD Data Collection Functions
void sd_start_collection();
void sd_stop_collection();

/******************************************************************************
 *                               Global Vars
 *****************************************************************************/
// Scheduler / Tasks
Scheduler scheduler;
Task t1( 200, TASK_FOREVER, data_send_task );
Task t2( 1000, TASK_FOREVER, gps_send_task );
Task t3( 100, TASK_FOREVER, data_collect_task );

// Xbee object
Xbee xbee( &Serial1 );

// Adc object
Adafruit_MCP3008 adc;

// GPS object
Adafruit_GPS gps( &Serial2 );

// IMU Object
// Adafruit_BNO055 imu_sensor = Adafruit_BNO055();
Adafruit_BNO055 imu_sensor = Adafruit_BNO055( -1, BNO055_ADDRESS_B );

// Pressure Sensor object
AllSensors_DLV_015A pressure_sensor = AllSensors_DLV_015A( &Wire );

//File objects and data string 
File snsr_file;
File gps_file;
File count_file;

// Store all sensor data in this structure.
data_pkg_t sensor_data;

bool logging_data;

/******************************************************************************
*                                 Procedures
******************************************************************************/

/**********************************************************
*   setup
*       Called once at startup before going into the main
*       loop.
**********************************************************/
void setup()
{
    logging_data = false;

    // Serial initialization (DEBUGING)
    Serial.begin( 9600 );

    // Xbee initialization
    xbee.setup( 9600 );

    // ADC initialization
    adc.begin( ADC_SS_PIN );

    //SD initialization 
    SD.begin( SD_SS_PIN );
    
    // GPS initialization
    gps.begin( 9600 );
    gps.sendCommand( PMTK_SET_NMEA_OUTPUT_RMCGGA ); // turn on RMC (recommended minimum) and GGA (fix data) including altitude
    gps.sendCommand( PMTK_SET_NMEA_UPDATE_1HZ );    // Set the update rate
    gps.sendCommand( PGCMD_NOANTENNA );             // Turn off updates on antenna status

    // IMU setup
    imu_sensor.begin();
    delay(1000);
    imu_sensor.setExtCrystalUse( true );

    // Humidity setup
    pressure_sensor.setPressureUnit( AllSensors_DLV::PSI );
    pressure_sensor.setTemperatureUnit( AllSensors_DLV::FAHRENHEIT );

    // Setup cooperative scheduler
    scheduler.init();
    scheduler.addTask( t1 );
    scheduler.addTask( t2 );
    scheduler.addTask( t3 );
    t1.enable();
    t2.enable();
    t3.enable();
}


/**********************************************************
*   loop
*       Main program loop
**********************************************************/
void loop()
{
    scheduler.execute();

    /******************************************************
    *  Receive data from xbee and parse it.
    ******************************************************/
    xbee.read();
    if( xbee.new_data_received() )
    {
        data_type_t data_type;
        uint8_t data[ MAX_DATA_LENGTH ];
        uint8_t size;

        xbee.get_data( data_type, data, size );

        switch( data_type )
        {
            case DATA_LOG:
            {
                data_log_sts_t sts = (data_log_sts_t)*data;

                switch( sts )
                {
                    case DATA_LOG_STS_START:
                        if( !logging_data )
                        {
                            logging_data = true;
                            sd_start_collection();
                        }
                        break;
                
                    case DATA_LOG_STS_STOP:
                        if( logging_data )
                        {
                            logging_data = false;
                            sd_stop_collection();
                        }
                        break;

                    default:
                        break;
                }
                
                xbee.send_data( DATA_LOG, (uint8_t*)&sts, sizeof(data_log_sts_t) );
                break;
            }

            default:
                break;
        }

    }

    /******************************************************
    *  Receive data from GPS and parse it.
    ******************************************************/
    gps.read();
    if( gps.newNMEAreceived() ) 
    {    
        gps.parse( gps.lastNMEA() );
    }
}


/**********************************************************
*   data_collect_task
*       100ms task. Collects data from various sensors.
**********************************************************/
void data_collect_task()
{
    char snsr_data_string[ 256 ];

    imu::Vector<3> eul_vec;
    imu::Vector<3> acc_vec;
    imu::Quaternion quat;

    // Get IMU data
    eul_vec = imu_sensor.getVector( Adafruit_BNO055::VECTOR_EULER );
    acc_vec = imu_sensor.getVector( Adafruit_BNO055::VECTOR_LINEARACCEL );

    sensor_data.angle_x = (float)eul_vec.x();
    sensor_data.angle_y = (float)eul_vec.y();
    sensor_data.angle_z = (float)eul_vec.z();
    
    sensor_data.accel_x = (float)acc_vec.x();
    sensor_data.accel_y = (float)acc_vec.y();
    sensor_data.accel_z = (float)acc_vec.z();

    // Get ADC data
    sensor_data.adc_chnl_0 = adc.readADC( 0 );
    sensor_data.adc_chnl_1 = adc.readADC( 1 );
    sensor_data.adc_chnl_2 = adc.readADC( 2 );
    sensor_data.adc_chnl_3 = adc.readADC( 3 );
    sensor_data.adc_chnl_4 = adc.readADC( 4 );
    sensor_data.adc_chnl_5 = adc.readADC( 5 );
    sensor_data.adc_chnl_6 = adc.readADC( 6 );
    sensor_data.adc_chnl_7 = adc.readADC( 7 );

    // Get humidity data
    pressure_sensor.readData();
    sensor_data.prsur = pressure_sensor.pressure;
    sensor_data.prsur_temp = pressure_sensor.temperature;

    if( ( snsr_file     )
     && ( logging_data  ) )
    {
        // Save Data to SD card
        sprintf( snsr_data_string,
                "%s, %s, %s, %s, %s, %s, %d, %d, %d, %d, %d, %d, %d, %d, %s, %s",
                String( sensor_data.angle_x, 4 ).c_str(),
                String( sensor_data.angle_y, 4 ).c_str(), 
                String( sensor_data.angle_z, 4 ).c_str(), 
                String( sensor_data.accel_x, 4 ).c_str(), 
                String( sensor_data.accel_y, 4 ).c_str(), 
                String( sensor_data.accel_z, 4 ).c_str(),
                sensor_data.adc_chnl_0,
                sensor_data.adc_chnl_1,
                sensor_data.adc_chnl_2,
                sensor_data.adc_chnl_3,
                sensor_data.adc_chnl_4,
                sensor_data.adc_chnl_5,
                sensor_data.adc_chnl_6,
                sensor_data.adc_chnl_7,
                String( sensor_data.prsur, 4 ).c_str(),
                String( sensor_data.prsur_temp, 4 ).c_str()
        );

        snsr_file.println( snsr_data_string );
    }
}


/**********************************************************
*   data_send_task
*       100ms task. Sends out data to ground
*       station. This is separate from the data collection
*       task so that we can send out data at a slower rate
*       than at which it is collected.
**********************************************************/
void data_send_task()
{
    xbee.send_data( SENSOR_DATA, (uint8_t*)&sensor_data, sizeof(sensor_data) );
}

/**********************************************************
*   gps_send_task
*       1000ms task. Sends the date, time, long and
*       lat from GPS to ground station.
**********************************************************/
void gps_send_task()
{
    gps_data_t data;
    char gps_data_string[ 150 ];

    data.year       = gps.year;
    data.month      = gps.month;
    data.day        = gps.day;
    data.hour       = gps.hour;
    data.min        = gps.minute;
    data.sec        = gps.seconds;
    data.lat        = gps.latitudeDegrees;
    data.lon        = gps.longitudeDegrees;
    data.fix        = gps.fix;
    data.fix_qual   = gps.fixquality;
    data.sat_num    = gps.satellites;

    xbee.send_data( GPS_DATA, (uint8_t*)&data, sizeof(data) );

    if( ( gps_file     )
     && ( logging_data ) )
    {
        //Save Data To SD card 
        sprintf( gps_data_string, 
                "%d, %d, %d, %d, %d, %d, %s, %s, %d, %d, %d",
                data.year, 
                data.month, 
                data.day, 
                data.hour,
                data.min,
                data.sec,
                String( data.lat, 4 ).c_str(), 
                String( data.lon, 4 ).c_str(),
                data.fix,
                data.fix_qual,
                data.sat_num   );

        gps_file.println( gps_data_string );

        gps_file.flush();
        snsr_file.flush();
    }
}

/**********************************************************
*   sd_start_collection
*       Initializes the files for the SD card to 
*       write to 
**********************************************************/
void sd_start_collection()
{
    char base_dir[15];
    char cnt_file_path[ 15 ];
    char snsr_file_path[ 25 ];
    char gps_file_path[ 25 ];
    uint8_t file_cnt;
    uint8_t day;
    uint8_t month;

    day = gps.day;
    month = gps.month;
    file_cnt = 0;

    // Create the base dir
    sprintf( base_dir, "/%d_%d", month, day );
    SD.mkdir( base_dir );

    // Get the log cnt in the base dir
    sprintf( cnt_file_path, "%s/cnt.txt", base_dir );
    if( SD.exists( cnt_file_path ) )
    {
        count_file = SD.open( cnt_file_path, ( O_WRITE | O_READ ) );
        file_cnt = count_file.parseInt();
        
        count_file.seek( 0 );
        count_file.println( ++file_cnt );
        count_file.close();
    }
    //Count file does not exist and needs to be initialized
    else
    {
        file_cnt = 1;
        count_file = SD.open( cnt_file_path, ( O_WRITE | O_READ | O_CREAT ) );
        count_file.println( file_cnt );
        count_file.close();
    }


    //Initialize the Sensor Data File
    sprintf( snsr_file_path, "%s/snsr_%d.csv", base_dir, file_cnt );

    snsr_file = SD.open( snsr_file_path, ( O_WRITE | O_CREAT | O_TRUNC ) );
    if( snsr_file )
    {
        snsr_file.println( "angle_X, angle_Y, angle_Z, accel_X, accel_Y, accel_Z, adc_chnl_0, adc_chnl_1, adc_chnl_2, adc_chnl_3, adc_chnl_4, adc_chnl_5, adc_chnl_6, adc_chnl_7, air pressure, air pressure temp" );
        snsr_file.flush();
    }
    
    //Initialize GPS data file 
    sprintf(gps_file_path, "%s/gps_%d.csv", base_dir, file_cnt );
    
    gps_file = SD.open( gps_file_path, ( O_WRITE | O_CREAT | O_TRUNC ) );
    if( gps_file )
    {
        gps_file.println( "year, month, day, hour, minute, second, latitude, longitude, fix, fix quality, satellites" );
        gps_file.flush();
    }


}
/**********************************************************
*   sd_stop_collection
*       This function will write any remaining data 
*       and close files 
**********************************************************/
void sd_stop_collection()
{
    snsr_file.close();
    gps_file.close();    

}