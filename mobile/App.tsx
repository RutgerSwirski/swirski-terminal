import React, { useEffect, useState, useRef } from 'react';
import { Button, StyleSheet, Text, View } from 'react-native';
import { State } from 'react-native-ble-plx';
import type { Device, Subscription } from 'react-native-ble-plx';
import { bleManager } from './src/ble/bleManager';
import { requestBlePermissions } from './src/ble/requestBlePermissions';

import {
  SERVICE_UUID,
  TX_CHARACTERISTIC_UUID,
  RX_CHARACTERISTIC_UUID,
} from './src/ble/constants';

import { decodeBase64ToUtf8, encodeUtf8ToBase64 } from './src/ble/encoding';

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

  const txSubscriptionRef = useRef<Subscription | null>(null);
  const disconnectSubscriptionRef = useRef<Subscription | null>(null);

  async function sendPing() {
    if (!connectedDevice || connectionStatus !== 'ready') {
      console.log('Not connected');
      return;
    }

    const pingMessage = {
      version: 1,
      type: 'ping',
      id: `mobile-${Date.now()}`,
    };
    const json = JSON.stringify(pingMessage);

    const encoded = encodeUtf8ToBase64(json);

    console.log('Sending ping:', encoded);

    try {
      await bleManager.writeCharacteristicWithResponseForDevice(
        connectedDevice.id,
        SERVICE_UUID,
        RX_CHARACTERISTIC_UUID,
        encoded,
      );

      console.log('Ping sent');
    } catch (error) {
      console.log('Error sending ping:', error);
    }
  }

  function subscribeToTx(device: Device) {
    txSubscriptionRef.current?.remove();

    txSubscriptionRef.current = bleManager.monitorCharacteristicForDevice(
      device.id,
      SERVICE_UUID,
      TX_CHARACTERISTIC_UUID,
      (error, characteristic) => {
        if (error) {
          console.log(error);
          return;
        }

        if (!characteristic?.value) {
          return;
        }

        const message = decodeBase64ToUtf8(characteristic.value);

        console.log('Received TX message:', message);
      },
    );

    console.log('Subscribed to TX characteristic');
  }

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
      bleManager.stopDeviceScan();
      setIsScanning(false);

      setConnectionStatus('connecting');

      const connected = await device.connect();

      console.log('Connected to device:', connected.name, connected.id);

      const mtuDevice = await connected.requestMTU(128);

      console.log('Negotiated MTU:', mtuDevice.mtu);

      setConnectionStatus('discovering');

      const discovered =
        await mtuDevice.discoverAllServicesAndCharacteristics();

      await inspectGatt(discovered);

      subscribeToTx(discovered);

      disconnectSubscriptionRef.current?.remove();

      disconnectSubscriptionRef.current = bleManager.onDeviceDisconnected(
        discovered.id,
        error => {
          if (error) {
            console.log('BLE disconnected:', error);
          }

          txSubscriptionRef.current?.remove();
          txSubscriptionRef.current = null;

          setConnectedDevice(null);
          setConnectionStatus('disconnected');
        },
      );

      setConnectedDevice(discovered);
      setConnectionStatus('ready');
    } catch (error) {
      console.error('BLE connection error:', error);

      txSubscriptionRef.current?.remove();
      txSubscriptionRef.current = null;

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
    const stateSubscription = bleManager.onStateChange(nextState => {
      console.log('BLE state:', nextState);

      setBleState(nextState);
    }, true);

    return () => {
      stateSubscription.remove();
      txSubscriptionRef.current?.remove();
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

      {connectedDevice && (
        <View>
          <Button
            title="Ping!"
            onPress={sendPing}
            disabled={connectionStatus !== 'ready'}
          />
        </View>
      )}
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
