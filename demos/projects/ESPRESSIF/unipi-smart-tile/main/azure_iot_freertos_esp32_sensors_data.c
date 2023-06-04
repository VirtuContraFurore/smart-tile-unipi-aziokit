/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#include "azure_iot_freertos_esp32_sensors_data.h"
#include "steps_counter.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* Azure Provisioning/IoT Hub library includes */
#include "azure_iot_hub_client.h"
#include "azure_iot_hub_client_properties.h"

#include "sample_azure_iot_pnp_data_if.h"
#include "sensor_manager.h"
/*-----------------------------------------------------------*/

#define INDEFINITE_TIME    ( ( time_t ) -1 )
#define lengthof( x )                  ( sizeof( x ) - 1 )
/* This macro helps remove quotes around a string. */
/* That is achieved by skipping the first char in the string, and reducing the length by 2 chars. */
#define UNQUOTE_STRING( x )            ( x + 1 )
#define UNQUOTED_STRING_LENGTH( n )    ( n - 2 )
/*-----------------------------------------------------------*/

static const char * TAG = "sample_azureiotkit";
/*-----------------------------------------------------------*/

/**
 * @brief Device Info Values
 */
#define sampleazureiotkitDEVICE_INFORMATION_NAME                  ( "deviceInformation" )
#define sampleazureiotkitMANUFACTURER_PROPERTY_NAME               ( "manufacturer" )
#define sampleazureiotkitMODEL_PROPERTY_NAME                      ( "model" )
#define sampleazureiotkitSOFTWARE_VERSION_PROPERTY_NAME           ( "swVersion" )
#define sampleazureiotkitOS_NAME_PROPERTY_NAME                    ( "osName" )
#define sampleazureiotkitPROCESSOR_ARCHITECTURE_PROPERTY_NAME     ( "processorArchitecture" )
#define sampleazureiotkitPROCESSOR_MANUFACTURER_PROPERTY_NAME     ( "processorManufacturer" )
#define sampleazureiotkitTOTAL_STORAGE_PROPERTY_NAME              ( "totalStorage" )
#define sampleazureiotkitTOTAL_MEMORY_PROPERTY_NAME               ( "totalMemory" )

#define sampleazureiotkitMANUFACTURER_PROPERTY_VALUE              ( "ESPRESSIF" )
#define sampleazureiotkitMODEL_PROPERTY_VALUE                     ( "ESP32 Azure IoT Kit" )
#define sampleazureiotkitVERSION_PROPERTY_VALUE                   ( "1.0.0" )
#define sampleazureiotkitOS_NAME_PROPERTY_VALUE                   ( "FreeRTOS" )
#define sampleazureiotkitARCHITECTURE_PROPERTY_VALUE              ( "ESP32 WROVER-B" )
#define sampleazureiotkitPROCESSOR_MANUFACTURER_PROPERTY_VALUE    ( "ESPRESSIF" )
/* The next couple properties are in KiloBytes. */
#define sampleazureiotkitTOTAL_STORAGE_PROPERTY_VALUE             4096
#define sampleazureiotkitTOTAL_MEMORY_PROPERTY_VALUE              8192

/**
 * @brief Telemetry Values
 */
#define telemetry_STEPS                 ( "step_count" )
#define telemetry_STEP_DURATION_MS      ( "StepDuration_ms" )
#define telemetry_STEP_ACCEL_PEAK       ( "StepAccelerationPeak" )
#define telemetry_HARVESTED_ENERGY      ( "HarvestedEnergy" )
#define telemetry_EVENT_TIME            ( "EventTimeUnixTime" )

static time_t xLastTelemetrySendTime = INDEFINITE_TIME;

/**
 * @brief Command Values
 */
#define sampleazureiotCOMMAND_EMPTY_PAYLOAD    "{}"

static const char sampleazureiotCOMMAND_RESET_STEPS_COUNTER[] = "ResetStepsCounter";

/**
 * @brief Property Values for teletry frequency (expressed in seconds)
 */
#define sampleazureiotPROPERTY_STATUS_SUCCESS         200
#define sampleazureiotPROPERTY_SUCCESS                "success"
#define sampleazureiotPROPERTY_TELEMETRY_FREQUENCY    ( "telemetryFrequencySecs" )
#define sampleazureiotkitDEFAULT_TELEMETRY_FREQUENCY  20

static int lTelemetryFrequencySecs = sampleazureiotkitDEFAULT_TELEMETRY_FREQUENCY;
/*-----------------------------------------------------------*/

int32_t lGenerateDeviceInfo( uint8_t * pucPropertiesData,
                             uint32_t ulPropertiesDataSize )
{
    AzureIoTResult_t xAzIoTResult;
    AzureIoTJSONWriter_t xWriter;
    int32_t lBytesWritten;

    /* Update reported property */
    xAzIoTResult = AzureIoTJSONWriter_Init( &xWriter, pucPropertiesData, ulPropertiesDataSize );
    configASSERT( xAzIoTResult == eAzureIoTSuccess );

    xAzIoTResult = AzureIoTJSONWriter_AppendBeginObject( &xWriter );
    configASSERT( xAzIoTResult == eAzureIoTSuccess );

    xAzIoTResult = AzureIoTHubClientProperties_BuilderBeginComponent( &xAzureIoTHubClient, &xWriter, ( const uint8_t * ) sampleazureiotkitDEVICE_INFORMATION_NAME, strlen( sampleazureiotkitDEVICE_INFORMATION_NAME ) );
    configASSERT( xAzIoTResult == eAzureIoTSuccess );

    xAzIoTResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotkitMANUFACTURER_PROPERTY_NAME, lengthof( sampleazureiotkitMANUFACTURER_PROPERTY_NAME ),
                                                                     ( uint8_t * ) sampleazureiotkitMANUFACTURER_PROPERTY_VALUE, lengthof( sampleazureiotkitMANUFACTURER_PROPERTY_VALUE ) );
    configASSERT( xAzIoTResult == eAzureIoTSuccess );

    xAzIoTResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotkitMODEL_PROPERTY_NAME, lengthof( sampleazureiotkitMODEL_PROPERTY_NAME ),
                                                                     ( uint8_t * ) sampleazureiotkitMODEL_PROPERTY_VALUE, lengthof( sampleazureiotkitMODEL_PROPERTY_VALUE ) );
    configASSERT( xAzIoTResult == eAzureIoTSuccess );

    xAzIoTResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotkitSOFTWARE_VERSION_PROPERTY_NAME, lengthof( sampleazureiotkitSOFTWARE_VERSION_PROPERTY_NAME ),
                                                                     ( uint8_t * ) sampleazureiotkitVERSION_PROPERTY_VALUE, lengthof( sampleazureiotkitVERSION_PROPERTY_VALUE ) );
    configASSERT( xAzIoTResult == eAzureIoTSuccess );

    xAzIoTResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotkitOS_NAME_PROPERTY_NAME, lengthof( sampleazureiotkitOS_NAME_PROPERTY_NAME ),
                                                                     ( uint8_t * ) sampleazureiotkitOS_NAME_PROPERTY_VALUE, lengthof( sampleazureiotkitOS_NAME_PROPERTY_VALUE ) );
    configASSERT( xAzIoTResult == eAzureIoTSuccess );

    xAzIoTResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotkitPROCESSOR_ARCHITECTURE_PROPERTY_NAME, lengthof( sampleazureiotkitPROCESSOR_ARCHITECTURE_PROPERTY_NAME ),
                                                                     ( uint8_t * ) sampleazureiotkitARCHITECTURE_PROPERTY_VALUE, lengthof( sampleazureiotkitARCHITECTURE_PROPERTY_VALUE ) );
    configASSERT( xAzIoTResult == eAzureIoTSuccess );

    xAzIoTResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotkitPROCESSOR_MANUFACTURER_PROPERTY_NAME, lengthof( sampleazureiotkitPROCESSOR_MANUFACTURER_PROPERTY_NAME ),
                                                                     ( uint8_t * ) sampleazureiotkitPROCESSOR_MANUFACTURER_PROPERTY_VALUE, lengthof( sampleazureiotkitPROCESSOR_MANUFACTURER_PROPERTY_VALUE ) );
    configASSERT( xAzIoTResult == eAzureIoTSuccess );

    xAzIoTResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( &xWriter, ( uint8_t * ) sampleazureiotkitTOTAL_STORAGE_PROPERTY_NAME, lengthof( sampleazureiotkitTOTAL_STORAGE_PROPERTY_NAME ),
                                                                     sampleazureiotkitTOTAL_STORAGE_PROPERTY_VALUE, 0 );
    configASSERT( xAzIoTResult == eAzureIoTSuccess );

    xAzIoTResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( &xWriter, ( uint8_t * ) sampleazureiotkitTOTAL_MEMORY_PROPERTY_NAME, lengthof( sampleazureiotkitTOTAL_MEMORY_PROPERTY_NAME ),
                                                                     sampleazureiotkitTOTAL_MEMORY_PROPERTY_VALUE, 0 );
    configASSERT( xAzIoTResult == eAzureIoTSuccess );

    xAzIoTResult = AzureIoTHubClientProperties_BuilderEndComponent( &xAzureIoTHubClient, &xWriter );
    configASSERT( xAzIoTResult == eAzureIoTSuccess );

    xAzIoTResult = AzureIoTJSONWriter_AppendEndObject( &xWriter );
    configASSERT( xAzIoTResult == eAzureIoTSuccess );

    lBytesWritten = AzureIoTJSONWriter_GetBytesUsed( &xWriter );

    if( lBytesWritten < 0 )
    {
        LogError( ( "Error getting the bytes written for the device properties JSON" ) );
        lBytesWritten = 0;
    }

    return lBytesWritten;
}
/*-----------------------------------------------------------*/

/**
 * @brief Command message callback handler
 */
uint32_t ulSampleHandleCommand( AzureIoTHubClientCommandRequest_t * pxMessage,
                                uint32_t * pulResponseStatus,
                                uint8_t * pucCommandResponsePayloadBuffer,
                                uint32_t ulCommandResponsePayloadBufferSize )
{
    uint32_t ulCommandResponsePayloadLength;

    ESP_LOGI( TAG, "Command payload : %.*s \r\n",
              ( int16_t ) pxMessage->ulPayloadLength,
              ( const char * ) pxMessage->pvMessagePayload );

    if( strncmp( ( const char * ) pxMessage->pucCommandName, sampleazureiotCOMMAND_RESET_STEPS_COUNTER, pxMessage->usCommandNameLength ) == 0 )
    {
        steps_counter_reset_steps();

        *pulResponseStatus = AZ_IOT_STATUS_OK;
        ulCommandResponsePayloadLength = lengthof( sampleazureiotCOMMAND_EMPTY_PAYLOAD );
        ( void ) memcpy( pucCommandResponsePayloadBuffer, sampleazureiotCOMMAND_EMPTY_PAYLOAD, ulCommandResponsePayloadLength );
    }
    else
    {
        *pulResponseStatus = AZ_IOT_STATUS_NOT_FOUND;
        ulCommandResponsePayloadLength = lengthof( sampleazureiotCOMMAND_EMPTY_PAYLOAD );
        configASSERT( ulCommandResponsePayloadBufferSize >= ulCommandResponsePayloadLength );
        ( void ) memcpy( pucCommandResponsePayloadBuffer, sampleazureiotCOMMAND_EMPTY_PAYLOAD, ulCommandResponsePayloadLength );
    }

    return ulCommandResponsePayloadLength;
}
/*-----------------------------------------------------------*/

uint32_t ulSampleCreateTelemetry( uint8_t * pucTelemetryData,
                                  uint32_t ulTelemetryDataLength )
{
    int32_t lBytesWritten = 0;
    time_t xNow = time( NULL );

    if( xNow == INDEFINITE_TIME )
    {
        ESP_LOGE( TAG, "Failed obtaining current time.\r\n" );
    }

    if( ( xLastTelemetrySendTime == INDEFINITE_TIME ) || ( xNow == INDEFINITE_TIME ) ||
        ( difftime( xNow, xLastTelemetrySendTime ) > lTelemetryFrequencySecs ) )
    {
        int32_t steps;
        float accel_peak;
        int32_t step_duration_ms;
        float step_energy;

        /* Fetch data and check if there is a new event to be sent */
        if(steps_counter_get_data(&steps, &accel_peak, &step_duration_ms, &step_energy) != 1)
            return 0;

        /* Start building the response JSON */
        AzureIoTResult_t xAzIoTResult;
        AzureIoTJSONWriter_t xWriter;

        /* Initialize Json Writer */
        xAzIoTResult = AzureIoTJSONWriter_Init( &xWriter, pucTelemetryData, ulTelemetryDataLength );
        configASSERT( xAzIoTResult == eAzureIoTSuccess );

        xAzIoTResult = AzureIoTJSONWriter_AppendBeginObject( &xWriter );
        configASSERT( xAzIoTResult == eAzureIoTSuccess );

        /* Write steps count */
        configASSERT( AzureIoTJSONWriter_AppendPropertyWithInt32Value( &xWriter, ( uint8_t * ) telemetry_STEPS, lengthof( telemetry_STEPS ), steps ) == eAzureIoTSuccess );

        /* Write step duration ms */
        configASSERT( AzureIoTJSONWriter_AppendPropertyWithInt32Value( &xWriter, ( uint8_t * ) telemetry_STEP_DURATION_MS, lengthof( telemetry_STEP_DURATION_MS ), step_duration_ms ) == eAzureIoTSuccess );

        /* Write acceleration peak */
        configASSERT( AzureIoTJSONWriter_AppendPropertyWithDoubleValue( &xWriter, ( uint8_t * ) telemetry_STEP_ACCEL_PEAK, lengthof( telemetry_STEP_ACCEL_PEAK ), accel_peak, 3) == eAzureIoTSuccess );

        /* Write Harvested energy */
        configASSERT( AzureIoTJSONWriter_AppendPropertyWithDoubleValue( &xWriter, ( uint8_t * ) telemetry_HARVESTED_ENERGY, lengthof( telemetry_HARVESTED_ENERGY ), step_energy, 3) == eAzureIoTSuccess );

        /* Event time */
        configASSERT( AzureIoTJSONWriter_AppendPropertyWithInt32Value( &xWriter, ( uint8_t * ) telemetry_EVENT_TIME, lengthof( telemetry_EVENT_TIME ), (int32_t ) time( NULL ) ) == eAzureIoTSuccess );

        /* Complete Json Content */
        xAzIoTResult = AzureIoTJSONWriter_AppendEndObject( &xWriter );
        configASSERT( xAzIoTResult == eAzureIoTSuccess );

        lBytesWritten = AzureIoTJSONWriter_GetBytesUsed( &xWriter );
        configASSERT( lBytesWritten > 0 );

        xLastTelemetrySendTime = xNow;
    }

    return lBytesWritten;
}
/*-----------------------------------------------------------*/


/**
 * @brief Acknowledges the update of Telemetry Frequency property.
 */
static uint32_t prvGenerateAckForTelemetryFrequencyPropertyUpdate( uint8_t * pucPropertiesData,
                                                                   uint32_t ulPropertiesDataSize,
                                                                   uint32_t ulVersion )
{
    AzureIoTResult_t xAzIoTResult;
    AzureIoTJSONWriter_t xWriter;
    int32_t lBytesWritten;

    xAzIoTResult = AzureIoTJSONWriter_Init( &xWriter, pucPropertiesData, ulPropertiesDataSize );
    configASSERT( xAzIoTResult == eAzureIoTSuccess );

    xAzIoTResult = AzureIoTJSONWriter_AppendBeginObject( &xWriter );
    configASSERT( xAzIoTResult == eAzureIoTSuccess );

    xAzIoTResult = AzureIoTHubClientProperties_BuilderBeginResponseStatus( &xAzureIoTHubClient,
                                                                           &xWriter,
                                                                           ( const uint8_t * ) sampleazureiotPROPERTY_TELEMETRY_FREQUENCY,
                                                                           lengthof( sampleazureiotPROPERTY_TELEMETRY_FREQUENCY ),
                                                                           sampleazureiotPROPERTY_STATUS_SUCCESS,
                                                                           ulVersion,
                                                                           ( const uint8_t * ) sampleazureiotPROPERTY_SUCCESS,
                                                                           lengthof( sampleazureiotPROPERTY_SUCCESS ) );
    configASSERT( xAzIoTResult == eAzureIoTSuccess );

    xAzIoTResult = AzureIoTJSONWriter_AppendInt32( &xWriter, lTelemetryFrequencySecs );
    configASSERT( xAzIoTResult == eAzureIoTSuccess );

    xAzIoTResult = AzureIoTHubClientProperties_BuilderEndResponseStatus( &xAzureIoTHubClient, &xWriter );
    configASSERT( xAzIoTResult == eAzureIoTSuccess );

    xAzIoTResult = AzureIoTJSONWriter_AppendEndObject( &xWriter );
    configASSERT( xAzIoTResult == eAzureIoTSuccess );

    lBytesWritten = AzureIoTJSONWriter_GetBytesUsed( &xWriter );
    configASSERT( lBytesWritten > 0 );

    return ( uint32_t ) lBytesWritten;
}
/*-----------------------------------------------------------*/

static void prvSkipPropertyAndValue( AzureIoTJSONReader_t * pxReader )
{
    AzureIoTResult_t xResult;

    xResult = AzureIoTJSONReader_NextToken( pxReader );
    configASSERT( xResult == eAzureIoTSuccess );

    xResult = AzureIoTJSONReader_SkipChildren( pxReader );
    configASSERT( xResult == eAzureIoTSuccess );

    xResult = AzureIoTJSONReader_NextToken( pxReader );
    configASSERT( xResult == eAzureIoTSuccess );
}
/*-----------------------------------------------------------*/

/**
 * The only managed property is TELEMETRY_FREQUENCY -> when changed, ack is sent by
 * calling prvGenerateAckForTelemetryFrequencyPropertyUpdate()
 * @brief Handler for writable properties updates.
 */
void vHandleWritableProperties( AzureIoTHubClientPropertiesResponse_t * pxMessage,
                                uint8_t * pucWritablePropertyResponseBuffer,
                                uint32_t ulWritablePropertyResponseBufferSize,
                                uint32_t * pulWritablePropertyResponseBufferLength )
{
    AzureIoTResult_t xAzIoTResult;
    AzureIoTJSONReader_t xJsonReader;
    const uint8_t * pucComponentName = NULL;
    uint32_t ulComponentNameLength = 0;
    uint32_t ulPropertyVersion;

    xAzIoTResult = AzureIoTJSONReader_Init( &xJsonReader, pxMessage->pvMessagePayload, pxMessage->ulPayloadLength );
    configASSERT( xAzIoTResult == eAzureIoTSuccess );

    xAzIoTResult = AzureIoTHubClientProperties_GetPropertiesVersion( &xAzureIoTHubClient, &xJsonReader, pxMessage->xMessageType, &ulPropertyVersion );
    configASSERT( xAzIoTResult == eAzureIoTSuccess );

    /* Reset JSON reader to the beginning */
    xAzIoTResult = AzureIoTJSONReader_Init( &xJsonReader, pxMessage->pvMessagePayload, pxMessage->ulPayloadLength );
    configASSERT( xAzIoTResult == eAzureIoTSuccess );

    while( ( xAzIoTResult = AzureIoTHubClientProperties_GetNextComponentProperty( &xAzureIoTHubClient, &xJsonReader,
                                                                                  pxMessage->xMessageType, eAzureIoTHubClientPropertyWritable,
                                                                                  &pucComponentName, &ulComponentNameLength ) ) == eAzureIoTSuccess )
    {
        if( AzureIoTJSONReader_TokenIsTextEqual( &xJsonReader,
                                                 ( const uint8_t * ) sampleazureiotPROPERTY_TELEMETRY_FREQUENCY,
                                                 lengthof( sampleazureiotPROPERTY_TELEMETRY_FREQUENCY ) ) )
        {
            xAzIoTResult = AzureIoTJSONReader_NextToken( &xJsonReader );
            configASSERT( xAzIoTResult == eAzureIoTSuccess );

            xAzIoTResult = AzureIoTJSONReader_GetTokenInt32( &xJsonReader, ( int32_t * ) &lTelemetryFrequencySecs );
            configASSERT( xAzIoTResult == eAzureIoTSuccess );

            *pulWritablePropertyResponseBufferLength = prvGenerateAckForTelemetryFrequencyPropertyUpdate(
                pucWritablePropertyResponseBuffer,
                ulWritablePropertyResponseBufferSize,
                ulPropertyVersion );

            ESP_LOGI( TAG, "Telemetry frequency set to once every %d seconds.\r\n", lTelemetryFrequencySecs );

            xAzIoTResult = AzureIoTJSONReader_NextToken( &xJsonReader );
            configASSERT( xAzIoTResult == eAzureIoTSuccess );
        }
        else
        {
            LogInfo( ( "Unknown property arrived: skipping over it." ) );

            /* Unknown property arrived. We have to skip over the property and value to continue iterating. */
            prvSkipPropertyAndValue( &xJsonReader );
        }
    }

    if( xAzIoTResult != eAzureIoTErrorEndOfProperties )
    {
        LogError( ( "There was an error parsing the properties: result 0x%08x", xAzIoTResult ) );
    }
    else
    {
        LogInfo( ( "Successfully parsed properties" ) );
    }
}
/*-----------------------------------------------------------*/

static bool xUpdateDeviceProperties = true; // this flag checks to generate the the device info only once

uint32_t ulSampleCreateReportedPropertiesUpdate( uint8_t * pucPropertiesData,
                                                 uint32_t ulPropertiesDataSize )
{
    /* No reported properties to send if length is zero. */
    uint32_t lBytesWritten = 0;

    if( xUpdateDeviceProperties )
    {
        lBytesWritten = lGenerateDeviceInfo( pucPropertiesData, ulPropertiesDataSize );
        configASSERT( lBytesWritten > 0 );
        xUpdateDeviceProperties = false;
    } // if false the buffer remains unchanged

    return lBytesWritten;
}
/*-----------------------------------------------------------*/
