import React, { useEffect, useState } from 'react';
import { Button, StyleSheet, Text, View } from 'react-native';
import { State } from 'react-native-ble-plx';
import type { Device } from 'react-native-ble-plx';
import { bleManager } from './src/ble/bleManager';
import { requestBlePermissions } from './src/ble/requestBlePermissions';

type ConnectionStatus =
  | 'disconnected'
  | 'connecting'
  | 'discovering'
  | 'ready'
  | 'error';

function App() {
  const [bleState, setBleState] = useState<State>(State.Unknown);
  const [isScanning, setIsScanning] = useState<boolean>(false);
  const [devices, setDevices] = useState<Device[]>([]);

  const [connectionStatus, setConnectionStatus] =
    useState<ConnectionStatus>('disconnected');

  const [connectedDevice, setConnectedDevice] = useState<Device | null>(null);

  async function inspectGatt(device: Device) {
    const services = await bleManager.servicesForDevice(device.id);

    for (const service of services) {
      console.log(`Service: ${service.uuid}`);
      const characteristics = await bleManager.characteristicsForDevice(
        device.id,
        service.uuid,
      );

      for (const characteristic of characteristics) {
        console.log('Characteristic:', characteristic.uuid, {
          readable: characteristic.isReadable,
          writableWithResponse: characteristic.isWritableWithResponse,
          writableWithoutResponse: characteristic.isWritableWithoutResponse,
          notifiable: characteristic.isNotifiable,
        });
      }
    }
  }

  async function connectToDevice(device: Device) {
    try {
      bleManager.stopDeviceScan(); // stop scanning for devices

      setConnectionStatus('connecting');

      const connected = await device.connect();

      console.log('Connected to device:', connected.name, connected.id);

      setConnectionStatus('discovering');

      const discovered =
        await connected.discoverAllServicesAndCharacteristics();

      await inspectGatt(discovered);

      console.log('Discovered services and characteristics:', discovered.id);

      setConnectedDevice(discovered);
      setConnectionStatus('ready');
    } catch (error) {
      console.error('BLE connection error:', error);
      setConnectedDevice(null);
      setConnectionStatus('error');
    }
  }

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
    });

    setTimeout(() => {
      bleManager.stopDeviceScan();
      setIsScanning(false);
    }, 5000);
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
    <View style={styles.container}>
      <Text>Swirski Terminal</Text>
      <Text>Bluetooth: {bleState}</Text>

      <Text>Status: {connectionStatus}</Text>

      {connectedDevice && <Text>Connected: {connectedDevice.name}</Text>}

      <Button
        title={isScanning ? 'Scanning...' : 'Start scanning'}
        disabled={isScanning}
        onPress={startScan}
      />

      {devices.map(device => (
        <View key={device.id}>
          <Text>{device.name ?? device.localName ?? 'Unnamed device'}</Text>

          <Text>{device.id}</Text>
          <Text>RSSI: {device.rssi ?? 'Unknown'}</Text>

          <Button title="Connect" onPress={() => connectToDevice(device)} />
        </View>
      ))}
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#fff',
    alignItems: 'center',
    justifyContent: 'center',
  },
});
export default App;
