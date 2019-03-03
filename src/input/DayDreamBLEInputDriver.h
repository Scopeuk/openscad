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
#pragma once

#include "input/InputDriver.h"
#include <qbluetoothaddress.h>
#include <qbluetoothdevicediscoveryagent.h>
#include <qbluetoothlocaldevice.h>
#include <qbluetoothdeviceinfo.h>
#include <qbluetoothservicediscoveryagent.h>
#include <QBluetoothServiceDiscoveryAgent>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothDeviceInfo>
#include <QLowEnergyController>
#include <QBluetoothUuid>
#include <QLowEnergyService>
#include <QBluetoothServiceInfo>
#include <QList>
#include <QTimer>
#include <QtMath>
#include <QLoggingCategory>
#include <QQuaternion>

class DayDreamBLEInputDriver : public InputDriver
{
    Q_OBJECT

   QLowEnergyCharacteristic  dayDreamServiceCharacteristic;
   QLowEnergyController* dayDreamControler;
   QLowEnergyService* dayDreamService;
   QBluetoothDeviceDiscoveryAgent* discoveryAgent;


public:
   const QBluetoothUuid characteristicUuid = QBluetoothUuid(QString("00000001-1000-1000-8000-00805f9b34fb"));
   const QBluetoothUuid serviceUuid = QBluetoothUuid(quint16(0xfe55));
    DayDreamBLEInputDriver();
    ~DayDreamBLEInputDriver();
    void run() override;
    bool open() override;
    void close() override;
    struct remoteState{
        bool isClickDown;
        bool isAppDown;
        bool isHomeDown;
        bool isVolPlusDown;
        bool isVolMinusDown;
        int time;
        int seq;
        double xOri;
        double yOri;
        double zOri;
        double xAcc;
        double yAcc;
        double zAcc;
        double xGyro;
        double yGyro;
        double zGyro;
        double xTouch;
        double yTouch;
    } remoteStateData, previousRemoveStateData;
    QQuaternion oriQuatenion;
    const std::string & get_name() const override;
    void updatedDataRecived(const QLowEnergyCharacteristic &characteristic, const QByteArray &newValue);
    void deviceFound(const QBluetoothDeviceInfo &info);
    void deviceConnected();
    void discoveryFinished();
	void serviceDiscovered(const QBluetoothUuid &service);
    void detailsDiscovered(QLowEnergyService::ServiceState newState);
    void deviceScanError(QBluetoothDeviceDiscoveryAgent::Error error);
	    int getButtonCount() const override{
        return 5;
    }
    int getAxisCount() const override{
        return 8;
    }


};

