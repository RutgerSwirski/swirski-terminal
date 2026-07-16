import React, { useEffect, useState } from 'react';
import { Button, SafeAreaView, Text, View } from 'react-native';
import { Device, State } from 'react-native-ble-plx';

import { bleManager } from './src/ble/bleManager';
import { requestBlePermissions } from './src/ble/requestBlePermissions';

function App() {
  const [bleState, setBleState] = useState<State>(State.Unknown);
  const [isScanning, setIsScanning] = useState<boolean>(false);
  const [devices, setDevices] = useState<Device[]>([]);

  const startScan = async () => {
    const hasPermission = await requestBlePermissions();

    if (!hasPermission) {
      console.log('BLE permission denied');
      return;
    }

    if (bleState !== State.PoweredOn) {
      console.log('BLE is not powered on');
      return;
    }

    setDevices([]);
    setIsScanning(true);

    bleManager.startDeviceScan(null, null, (error, device) => {
      if (error) {
        console.error('BLE scan error:', error);
        setIsScanning(false);
        return;
      }

      if (!device) {
        return;
      }

      console.log('Discovered device:', device);

      // const isSwirskiTerminal =
      //   device.name === 'Swirski Terminal' ||
      //   device.localName === 'Swirski Terminal';

      setDevices(currentDevices => {
        const alreadyExists = currentDevices.some(d => d.id === device.id);

        if (alreadyExists) {
          return currentDevices;
        }

        return [...currentDevices, device];
      });

      setTimeout(() => {
        bleManager.stopDeviceScan();
        setIsScanning(false);
      }, 5000);
    });
  };

  useEffect(() => {
    const subscription = bleManager.onStateChange(nextState => {
      console.log('BLE state:', nextState);
      setBleState(nextState);
    }, true);

    return () => {
      subscription.remove();
    };
  }, []);

  return (
    <SafeAreaView>
      <View>
        <Text>Swirski Terminal</Text>
        <Text>Bluetooth: {bleState}</Text>

        <Button
          title={isScanning ? 'Stop scanning' : 'Start scanning'}
          disabled={isScanning}
          onPress={startScan}
        />

        {devices.map(device => (
          <View key={device.id}>
            <Text>{device.name ?? device.localName ?? 'Unnamed device'}</Text>

            <Text>{device.id}</Text>
            <Text>RSSI: {device.rssi ?? 'Unknown'}</Text>
          </View>
        ))}
      </View>
    </SafeAreaView>
  );
}

export default App;
