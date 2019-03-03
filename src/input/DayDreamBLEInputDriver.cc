/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2017 Clifford Wolf <clifford@clifford.at> and
 *                          Marius Kintel <marius@kintel.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  As a special exception, you have permission to link this program
 *  with the CGAL library and distribute executables, as long as you
 *  follow the requirements of the GNU GPL in regard to all of the
 *  software in the executable aside from CGAL.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "input/DayDreamBLEInputDriver.h"
#include "input/InputDriverManager.h"





void DayDreamBLEInputDriver::run()
{
	PRINTD("Run Called");
	
}

DayDreamBLEInputDriver::DayDreamBLEInputDriver()
{
    PRINTD("Constructor Called");
//QLoggingCategory::setFilterRules("*.debug=false\n" "qt.bluetooth*=true"); 
 discoveryAgent = new QBluetoothDeviceDiscoveryAgent();
 discoveryAgent->setLowEnergyDiscoveryTimeout(5000);
 int timoutRet;
 timoutRet = discoveryAgent->lowEnergyDiscoveryTimeout();
 PRINTDB("Timeout = %i", timoutRet);
    connect(discoveryAgent, QOverload<QBluetoothDeviceDiscoveryAgent::Error>::of(&QBluetoothDeviceDiscoveryAgent::error),
       this, &DayDreamBLEInputDriver::deviceScanError);
    connect(discoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered,this,&DayDreamBLEInputDriver::deviceFound);
    discoveryAgent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
    if (discoveryAgent->isActive()) {
      PRINTD("Scanning Active");
    }
	PRINTD("Constructor Returned");
}

DayDreamBLEInputDriver::~DayDreamBLEInputDriver()
{
	PRINTD("Destructor Called");
}

bool DayDreamBLEInputDriver::open()
{
	PRINTD("Open Called");
    return true;
}

void DayDreamBLEInputDriver::close()
{
	PRINTD("Closed Called");

}

const std::string & DayDreamBLEInputDriver::get_name() const
{
    static std::string name = "DayDreamBLEInputDriver";
    return name;
}

void DayDreamBLEInputDriver::deviceFound(const QBluetoothDeviceInfo &info)
{
        PRINTD("device Found!");
        if (info.name() == "Daydream controller"){
            PRINTD("device found and stashed");
            dayDreamControler = QLowEnergyController::createCentral(info,this);
            dayDreamControler->setRemoteAddressType(QLowEnergyController::RandomAddress);
            //dayDreamControler->setRemoteAddressType(QLowEnergyController::PublicAddress);
            connect(dayDreamControler, &QLowEnergyController::connected, this, &DayDreamBLEInputDriver::deviceConnected);
            dayDreamControler->connectToDevice();
			PRINTD("connection started.");

        }
}

void DayDreamBLEInputDriver::deviceConnected()
{
            PRINTD("Device Connected");
            //dayDreamControler->createCentral(info);
            connect(dayDreamControler,&QLowEnergyController::discoveryFinished,this, &DayDreamBLEInputDriver::discoveryFinished);
            connect(dayDreamControler,&QLowEnergyController::serviceDiscovered,this, &DayDreamBLEInputDriver::serviceDiscovered);
			dayDreamControler->discoverServices();
}

void DayDreamBLEInputDriver::serviceDiscovered(const QBluetoothUuid &uuid)
{
            PRINTD("Service Discovered");
}


void DayDreamBLEInputDriver::discoveryFinished()
{
            PRINTD("Service Discovery Finsihed");
            dayDreamService = dayDreamControler->createServiceObject(serviceUuid);
            dayDreamService->discoverDetails();
            connect(dayDreamService, &QLowEnergyService::stateChanged, this, &DayDreamBLEInputDriver::detailsDiscovered);
}

void DayDreamBLEInputDriver::detailsDiscovered(QLowEnergyService::ServiceState newState)
{
   if(newState==QLowEnergyService::ServiceDiscovered){
            PRINTD("Details Discovered");
            dayDreamService->readCharacteristic(dayDreamService->characteristic(characteristicUuid));
            dayDreamServiceCharacteristic = dayDreamService->characteristic(characteristicUuid);
            connect(dayDreamService, &QLowEnergyService::characteristicChanged,this, &DayDreamBLEInputDriver::updatedDataRecived);
            //subscribe to status updates for data feed
            QLowEnergyDescriptor notification = dayDreamServiceCharacteristic.descriptor(QBluetoothUuid::ClientCharacteristicConfiguration);
            //enable status update messages from data feed on device
            if(notification.isValid()) dayDreamService->writeDescriptor(notification, QByteArray::fromHex("0100"));
    }
}




void DayDreamBLEInputDriver::updatedDataRecived(const QLowEnergyCharacteristic &characteristic, const QByteArray &newValue){
if(characteristic.uuid() != characteristicUuid) return;
        //PRINTD("Updated DataRecieved Called");
        //PRINTDB("new Data Length %d", newValue.size());
	//TODO: make more robust, check characteristic is the one we expect
        remoteStateData.isClickDown = (uint8_t(newValue.at(18))&0x1)>0;
        remoteStateData.isAppDown = (newValue.at(18) & 0x4) > 0;
        remoteStateData.isHomeDown = (newValue.at(18) & 0x2) > 0;
        remoteStateData.isVolPlusDown = (newValue.at(18) & 0x10) > 0;
        remoteStateData.isVolMinusDown = (newValue.at(18) & 0x8) > 0;

        remoteStateData.time = ((newValue.at(0) & 0xFF) << 1 | (newValue.at(1) & 0x80) >> 7);

        remoteStateData.seq = (newValue.at(1) & 0x7C) >> 2;

        int decoderTemp;
        decoderTemp = (newValue.at(1) & 0x03) << 11 | (newValue.at(2) & 0xFF) << 3 | (newValue.at(3) & 0x80) >> 5;
        decoderTemp = (decoderTemp << 19) >> 19;
        if (decoderTemp != 0)
            remoteStateData.xOri = decoderTemp * (360.0 / 4095.0);

        decoderTemp = (newValue.at(3) & 0x1F) << 8 | (newValue.at(4) & 0xFF);
        decoderTemp = (decoderTemp << 19) >> 19;
        if (decoderTemp != 0)
            remoteStateData.yOri = decoderTemp * (360.0 / 4095.0);

        decoderTemp = (newValue.at(5) & 0xFF) << 5 | (newValue.at(6) & 0xF8) >> 3;
        decoderTemp = (decoderTemp << 19) >> 19;
        if (decoderTemp != 0)
            remoteStateData.zOri = decoderTemp * (360.0 / 4095.0);

        decoderTemp = (newValue.at(6) & 0x07) << 10 | (newValue.at(7) & 0xFF) << 2 | (newValue.at(8) & 0xC0) >> 6;
        decoderTemp = (decoderTemp << 19) >> 19;
        if (decoderTemp != 0)
            remoteStateData.xAcc = decoderTemp * (8 / 4095.0);

        decoderTemp = (newValue.at(8) & 0x3F) << 7 | ((newValue.at(9) >> 1) & 0x7F);
        decoderTemp = (decoderTemp << 19) >> 19;
        if (decoderTemp != 0)
            remoteStateData.yAcc = decoderTemp * (8 / 4095.0);

        decoderTemp = (newValue.at(9) & 0x01) << 12 | (newValue.at(10) & 0xFF) << 4 | (newValue.at(11) & 0xF0) >> 4;
        decoderTemp = (decoderTemp << 19) >> 19;
        if (decoderTemp != 0)
            remoteStateData.zAcc = decoderTemp * (8 / 4095.0);

        decoderTemp = (newValue.at(11) & 0x0F) << 9 | (newValue.at(12) & 0xFF) << 1 | (newValue.at(13) & 0x80) >> 7;
        decoderTemp = (decoderTemp << 19) >> 19;
        if (decoderTemp != 0)
            remoteStateData.xGyro = decoderTemp / 4095.0;

        decoderTemp = (newValue.at(13) & 0x7F) << 6 | ((newValue.at(14) & 0xFC)>>2);
        decoderTemp = (decoderTemp << 19) >> 19;
        if (decoderTemp != 0)
            remoteStateData.yGyro = decoderTemp / 4095.0;

        decoderTemp = (newValue.at(14) & 0x7F) << 11 | (newValue.at(15) & 0xFF) << 3 | (newValue.at(16) & 0xE0) >> 5;
        decoderTemp = (decoderTemp << 19) >> 19;
        if (decoderTemp != 0)
            remoteStateData.zGyro = decoderTemp / 4095.0;

        remoteStateData.xTouch = (2.0*((newValue.at(16) & 0x1F) << 3 | (newValue.at(17) & 0xE0) >> 5) / 255.0)-1;
        if(remoteStateData.xTouch==-1) remoteStateData.xTouch = 0;
        remoteStateData.yTouch = -((2.0*((newValue.at(17) & 0x1F) << 3 | (newValue.at(18) & 0xE0) >> 5) / 255.0)-1);
        if(remoteStateData.yTouch==1) remoteStateData.yTouch = 0;

        //PRINTDB("Time %d",remoteStateData.time);
        //PRINTDB("seq %d",remoteStateData.seq);
        //PRINTDB("X accell value = %f",remoteStateData.xAcc);
        //PRINTDB("Y accell value = %f",remoteStateData.yAcc);
        //PRINTDB("Z accell value = %f",remoteStateData.zAcc);
        //PRINTDB("X ori value = %f",remoteStateData.xOri);
        //PRINTDB("Y ori value = %f",remoteStateData.yOri);
        //PRINTDB("Z ori value = %f",remoteStateData.zOri);
        //PRINTDB("X gyro value = %f",remoteStateData.xGyro);
        //PRINTDB("Y gyro value = %f",remoteStateData.yGyro);
        //PRINTDB("Z gyro value = %f",remoteStateData.zGyro);
        //PRINTDB("X track value = %f",remoteStateData.xTouch);
        //PRINTDB("Y track value = %f",remoteStateData.yTouch);
        oriQuatenion = QQuaternion::fromEulerAngles(remoteStateData.xOri, remoteStateData.zOri, remoteStateData.yOri);
		QQuaternion relativeOri = oriQuatenion;
        QVector4D q = relativeOri.toVector4D();
        float g[3] = {0,0,0};
        float compensated[3] = {0,0,0};
        //https://forum.arduino.cc/index.php?topic=447522.0
		//http://www.varesano.net/blog/fabio/simple-gravity-compensation-9-dom-imus
        g[0] = 2 * (q.x() * q.z() - q.w() * q.y());
        g[1] = 2 * (q.w() * q.x() + q.y() * q.z());
        g[2] = q.w() * q.w() - q.x() * q.x() - q.y() * q.y() + q.z() * q.z();


        compensated[0] = remoteStateData.xAcc + g[0];
        compensated[1] = remoteStateData.yAcc - g[2];
        compensated[2] = remoteStateData.zAcc + g[1];

		InputDriverManager::instance()->sendEvent(new InputEventAxisChanged(0, g[0]));
		InputDriverManager::instance()->sendEvent(new InputEventAxisChanged(1, g[1]));
		InputDriverManager::instance()->sendEvent(new InputEventAxisChanged(2, g[2]));
		InputDriverManager::instance()->sendEvent(new InputEventAxisChanged(3,remoteStateData.xAcc));
		InputDriverManager::instance()->sendEvent(new InputEventAxisChanged(4,remoteStateData.yAcc));
		InputDriverManager::instance()->sendEvent(new InputEventAxisChanged(5,remoteStateData.zAcc));
        //InputDriverManager::instance()->sendEvent(new InputEventAxisChanged(0,compensated[0]));
        //InputDriverManager::instance()->sendEvent(new InputEventAxisChanged(1,compensated[1]));
        //InputDriverManager::instance()->sendEvent(new InputEventAxisChanged(2,compensated[2]));
        //InputDriverManager::instance()->sendEvent(new InputEventAxisChanged(3,remoteStateData.xGyro));
        //InputDriverManager::instance()->sendEvent(new InputEventAxisChanged(4,remoteStateData.yGyro));
        //InputDriverManager::instance()->sendEvent(new InputEventAxisChanged(5,remoteStateData.zGyro));
        InputDriverManager::instance()->sendEvent(new InputEventAxisChanged(6,remoteStateData.xTouch));
        InputDriverManager::instance()->sendEvent(new InputEventAxisChanged(7,remoteStateData.yTouch));

        //check for change of button state
		if (remoteStateData.isClickDown != previousRemoveStateData.isClickDown) InputDriverManager::instance()->sendEvent(new InputEventButtonChanged(0, remoteStateData.isClickDown));
		if (remoteStateData.isAppDown != previousRemoveStateData.isAppDown) InputDriverManager::instance()->sendEvent(new InputEventButtonChanged(1, remoteStateData.isAppDown));
        if(remoteStateData.isHomeDown != previousRemoveStateData.isHomeDown) InputDriverManager::instance()->sendEvent(new InputEventButtonChanged(2, remoteStateData.isHomeDown));
        if(remoteStateData.isVolPlusDown != previousRemoveStateData.isVolPlusDown) InputDriverManager::instance()->sendEvent(new InputEventButtonChanged(3, remoteStateData.isVolPlusDown));
        if(remoteStateData.isVolMinusDown != previousRemoveStateData.isVolMinusDown) InputDriverManager::instance()->sendEvent(new InputEventButtonChanged(4, remoteStateData.isVolMinusDown));

        previousRemoveStateData = remoteStateData;


}

void DayDreamBLEInputDriver::deviceScanError(QBluetoothDeviceDiscoveryAgent::Error error)
{
	switch(error){
		case QBluetoothDeviceDiscoveryAgent::NoError :
			PRINTD("No error has occurred.");
			break;
		case QBluetoothDeviceDiscoveryAgent::PoweredOffError :
			PRINTD("The Bluetooth adaptor is powered off, power it on before doing discovery.");
			break;
		case QBluetoothDeviceDiscoveryAgent::InputOutputError :
			PRINTD("Writing or reading from the device resulted in an error.");
			break;
		case QBluetoothDeviceDiscoveryAgent::InvalidBluetoothAdapterError :
			PRINTD("The passed local adapter address does not match the physical adapter address of any local Bluetooth device.");
			break;
		case QBluetoothDeviceDiscoveryAgent::UnsupportedPlatformError :
			PRINTD("Device discovery is not possible or implemented on the current platform. The error is set in response to a call to start(). An example for such cases are iOS versions below 5.0 which do not support Bluetooth device search at all. This value was introduced by Qt 5.5.");
			break;
		case QBluetoothDeviceDiscoveryAgent::UnsupportedDiscoveryMethod :
			PRINTD("One of the requested discovery methods is not supported by the current platform. This value was introduced by Qt 5.8.");
			break;
		case QBluetoothDeviceDiscoveryAgent::UnknownError :
			PRINTD("An Unknown error has occured.");
			break;
	}

}
