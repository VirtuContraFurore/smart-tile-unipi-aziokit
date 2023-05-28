---
title: Connect an ESPRESSIF ESP-32 to Azure IoT Central quickstart
description: Use Azure IoT middleware for FreeRTOS to connect an ESPRESSIF ESP32-Azure IoT Kit device to Azure IoT and send telemetry.
author: timlt
ms.author: timlt
ms.service: iot-develop
ms.devlang: c
ms.topic: quickstart
ms.date: 11/29/2022
ms.custom: 
#Customer intent: As a device builder, I want to see a working IoT device sample connecting to Azure IoT, sending properties and telemetry, and responding to commands. As a solution builder, I want to use a tool to view the properties, commands, and telemetry an IoT Plug and Play device reports to the IoT hub it connects to.
---

# Quickstart: Connect an ESPRESSIF ESP32-Azure IoT Kit to IoT Central

**Total completion time**:  30 minutes

In this quickstart, you use the Azure IoT middleware for FreeRTOS to connect the ESPRESSIF ESP32-Azure IoT Kit (from now on, the ESP32 DevKit) to Azure IoT.

You'll complete the following tasks:

* Install a set of embedded development tools for programming an ESP32 DevKit
* Build an image and flash it onto the ESP32 DevKit
* Use Azure IoT Central to create cloud components, view properties, view device telemetry, and call direct commands

## Prerequisites

Operating system: Windows 10 or Windows 11

Hardware:
- ESPRESSIF [ESP32-Azure IoT Kit](https://www.espressif.com/products/devkits/esp32-azure-kit/overview)
- USB 2.0 A male to Micro USB male cable
- Wi-Fi 2.4 GHz
- An active Azure subscription. If you don't have an Azure subscription, create a [free account](https://azure.microsoft.com/free/?WT.mc_id=A261C142F) before you begin.

## Prepare the development environment

To set up your development environment, first you install the ESPRESSIF ESP-IDF build environment. The installer includes all the tools required to clone, build, flash, and monitor your device.

To install the ESP-IDF tools:
1. Download and launch the [ESP-IDF Online installer](https://dl.espressif.com/dl/esp-idf).
1. When the installer prompts for a version, select version ESP-IDF v5.0.x (x can be any)
1. When the installer prompts for the components to install, select only the stuff of ESP32 (not ESP32-C variants etc, those variants are likely to cause the installation to fail and are not need, so avoid them if you can)

---
 title: include file
 description: include file
 author: timlt
 ms.service: iot-develop
 ms.topic: include
 ms.date: 10/14/2022
 ms.author: timlt
 ms.custom: include file
---

## Create the cloud components

### Create the IoT Central application

There are several ways to connect devices to Azure IoT. In this section, you learn how to connect a device by using Azure IoT Central. IoT Central is an IoT application platform that reduces the cost and complexity of creating and managing IoT solutions.

To create a new application:

1. From [Azure IoT Central portal](https://apps.azureiotcentral.com/), select **Build** on the side navigation menu.

    > [!NOTE]
    > If you have an existing IoT Central application, you can use it to complete the steps in this article rather than create a new application. In this case, we recommend that you either create a new device or delete and recreate the device if you want to use an existing device ID.

1. Select **Create app** in the **Custom app** tile.
1. Add Application Name and a URL.
1. Select one of the standard pricing plans. Select your **Directory**, **Azure subscription**, and **Location**. To learn about creating IoT Central applications, see [Create an IoT Central application](../articles/iot-central/core/howto-create-iot-central-application.md). To learn about pricing, see [Azure IoT Central pricing](https://azure.microsoft.com/pricing/details/iot-central/).
1. Select **Create**. After IoT Central provisions the application, it redirects you automatically to the new application dashboard.

### Create a new device

In this section, you use the IoT Central application dashboard to create a new device. You'll use the connection information for the newly created device to securely connect your physical device in a later section.

To create a device:

1. From the application dashboard, select **Devices** on the side navigation menu.
1. Select **Create a device** from the **All devices** pane to open the **Create a new device** window. (If you're reusing an existing application that already has one or more devices, select **+ New** to open the window.)
1. Leave Device template as **Unassigned**. The template will be automatically recognized and assigned when the device first sends data. The json will contain an handle to the custom DTDL model used in this application
1. Fill in the desired Device name and Device ID.
1. Select the **Create** button.
1. The newly created device will appear in the **All devices** list.  Select on the device name to show details.
1. Select **Connect** in the top right menu bar to display the connection information used to configure the device in the next section.
1. Note the connection values for the following connection string parameters displayed in **Connect** dialog. You'll add these values to a configuration file in the next step:

    * `ID scope`
    * `Device ID`
    * `Primary key`

## Prepare the device
To connect the ESP32 DevKit to Azure, you'll modify configuration settings, build the image, and flash the image to the device. You can run all the commands in this section within the ESP-IDF command line.

### Set up the environment
To start the ESP-IDF PowerShell and clone the repo:
1. Select Windows **Start**, and launch **ESP-IDF PowerShell**. 
1. Navigate to a working folder where you want to clone the repo.
1. Clone the repo. This repo contains the Azure FreeRTOS middleware and sample code that you'll use to build an image for the ESP32 DevKit. 

    ```shell 
    git clone --recursive https://github.com/VirtuContraFurore/smart-tile-unipi-aziokit.git
    ```

To launch the ESP-IDF configuration settings:
1. In **ESP-IDF PowerShell**, navigate to the *iot-middleware-freertos-samples* directory that you cloned previously.
1. Navigate to the ESP32-Azure IoT Kit project directory *demos\projects\ESPRESSIF\unipi-smart-tile*.
1. Run the following command to launch the configuration menu:

    ```shell
    idf.py menuconfig
    ```

### Add configuration

To add configuration to connect to Azure IoT Central:
1. In **ESP-IDF PowerShell**, select **Azure IoT middleware for FreeRTOS Main Task Configuration --->**, and press Enter.
1. Select **Enable Device Provisioning Sample**, and press Enter to enable it.
1. Set the following Azure IoT configuration settings to the values that you saved after you created Azure resources.

    |Setting|Value|
    |-------------|-----|
    |**Azure IoT Device Symmetric Key** |{*Your primary key value*}|
    |**Azure Device Provisioning Service Registration ID** |{*Your Device ID value*}|
    |**Azure Device Provisioning Service ID Scope** |{*Your ID scope value*}|

1. Press Esc to return to the previous menu.

To add wireless network configuration:
1. Select **Azure IoT middleware for FreeRTOS Sample Configuration --->**, and press Enter.
1. Set the following configuration settings using your local wireless network credentials.

    |Setting|Value|
    |-------------|-----|
    |**WiFi SSID** |{*Your Wi-Fi SSID*}|
    |**WiFi Password** |{*Your Wi-Fi password*}|

1. Press Esc to return to the previous menu.

To save the configuration:
1. Press **S** to open the save options, then press Enter to save the configuration.
1. Press Enter to dismiss the acknowledgment message.
1. Press **Q** to quit the configuration menu.


### Build and flash the image
In this section, you use the ESP-IDF tools to build, flash, and monitor the ESP32 DevKit as it connects to Azure IoT.  

> [!NOTE]
> In the following commands in this section, use a short build output path near your root directory. Specify the build path after the `-B` parameter in each command that requires it. The short path helps to avoid a current issue in the ESPRESSIF ESP-IDF tools that can cause errors with long build path names.  The following commands use a local path *C:\espbuild* as an example.

To build the image:
1. In **ESP-IDF PowerShell**, from the *iot-middleware-freertos-samples\demos\projects\ESPRESSIF\unipi-smart-tile* directory, run the following command to build the image.

    ```shell
    idf.py --no-ccache -B "C:\espbuild" build 
    ```

1. After the build completes, confirm that the binary image file was created in the build path that you specified previously.

    *C:\espbuild\azure_iot_freertos_esp32.bin*

To flash the image:
1. On the ESP32 DevKit, locate the Micro USB port, which is highlighted in the following image:

1. Connect the Micro USB cable to the Micro USB port on the ESP32 DevKit, and then connect it to your computer.
1. Open Windows **Device Manager**, and view **Ports** to find out which COM port the ESP32 DevKit is connected to.
1. In **ESP-IDF PowerShell**, run the following command, replacing the *\<Your-COM-port\>* placeholder and brackets with the correct COM port from the previous step. For example, replace the placeholder with `COM3`. 

    ```shell
    idf.py --no-ccache -B "C:\espbuild" -p <Your-COM-port> flash
    ```

1. Confirm that the output completes with the following text for a successful flash:

    ```output
    Hash of data verified
    
    Leaving...
    Hard resetting via RTS pin...
    Done
    ```

To confirm that the device connects to Azure IoT Central:
1. In **ESP-IDF PowerShell**, run the following command to start the monitoring tool. As you did in a previous command, replace the \<Your-COM-port\> placeholder, and brackets with the COM port that the device is connected to.

    ```shell
    idf.py -B "C:\espbuild" -p <Your-COM-port> monitor
    ```

1. Check for repeating blocks of output similar to the following example. This output confirms that the device connects to Azure IoT and sends telemetry.

    ```output
    I (50807) AZ IOT: Successfully sent telemetry message
    I (50807) AZ IOT: Attempt to receive publish message from IoT Hub.
    
    I (51057) MQTT: Packet received. ReceivedBytes=2.
    I (51057) MQTT: Ack packet deserialized with result: MQTTSuccess.
    I (51057) MQTT: State record updated. New state=MQTTPublishDone.
    I (51067) AZ IOT: Puback received for packet id: 0x00000008
    I (53067) AZ IOT: Keeping Connection Idle...
    ```

## Verify the device status

To view the device status in the IoT Central portal:
1. From the application dashboard, select **Devices** on the side navigation menu.
1. Confirm that the **Device status** of the device is updated to **Provisioned**.
1. Confirm that the **Device template** of the device has updated to **Espressif ESP32 Azure IoT Kit**.

## View telemetry

In IoT Central, you can view the flow of telemetry from your device to the cloud.

To view telemetry in IoT Central:
1. From the application dashboard, select **Devices** on the side navigation menu.
1. Select the device from the device list.
1. Select the **Overview** tab on the device page, and view the telemetry as the device sends messages to the cloud.

## Send a command to the device

You can also use IoT Central to send a command to your device.

## View device information

You can view the device information from IoT Central.

Select the **About** tab on the device page.

> [!TIP]
> To customize these views, edit the [device template](../iot-central/core/howto-edit-device-template.md).

## Clean up resources

If you no longer need the Azure resources created in this tutorial, you can delete them from the IoT Central portal. Optionally, if you continue to another article in this Getting Started content, you can keep the resources you've already created and reuse them.

To keep the Azure IoT Central sample application but remove only specific devices:

1. Select the **Devices** tab for your application.
1. Select the device from the device list.
1. Select **Delete**.

To remove the entire Azure IoT Central sample application and all its devices and resources:

1. Select **Administration** > **Your application**.
1. Select **Delete**.

