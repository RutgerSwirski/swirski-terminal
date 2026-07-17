import { useCallback, useEffect, useRef, useState } from 'react';
import { BleErrorCode, State } from 'react-native-ble-plx';
import type { Device, Subscription } from 'react-native-ble-plx';

import { bleManager } from './bleManager';
import {
  RX_CHARACTERISTIC_UUID,
  SERVICE_UUID,
  TX_CHARACTERISTIC_UUID,
} from './constants';
import {
  BleFrameAssembler,
  decodeBase64ToBytes,
  encodeBytesToBase64,
  encodeMessageIntoFrames,
} from './framing';
import { requestBlePermissions } from './requestBlePermissions';
import { createDisconnectMessage, createPingMessage } from '../protocol/messages';

export type ConnectionStatus =
  | 'disconnected'
  | 'connecting'
  | 'discovering'
  | 'ready'
  | 'disconnecting'
  | 'error';

export type TransferProgress = {
  label: string;
  percent: number;
} | null;

function shouldLogFrameProgress(frameIndex: number, frameCount: number): boolean {
  return frameIndex === 1 || frameIndex === frameCount || frameIndex % 10 === 0;
}

function labelForMessage(message: Record<string, unknown>): string {
  return message.type === 'notifications.snapshot'
    ? 'Syncing notifications'
    : 'Sending';
}

export function useTerminalBle() {
  const [bleState, setBleState] = useState<State>(State.Unknown);
  const [isScanning, setIsScanning] = useState<boolean>(false);
  const [devices, setDevices] = useState<Device[]>([]);
  const [connectionStatus, setConnectionStatus] =
    useState<ConnectionStatus>('disconnected');
  const [connectedDevice, setConnectedDevice] = useState<Device | null>(null);
  const [transferProgress, setTransferProgress] =
    useState<TransferProgress>(null);

  const txSubscriptionRef = useRef<Subscription | null>(null);
  const disconnectSubscriptionRef = useRef<Subscription | null>(null);
  const txFrameAssemblerRef = useRef<BleFrameAssembler | null>(null);

  if (txFrameAssemblerRef.current === null) {
    txFrameAssemblerRef.current = new BleFrameAssembler();
  }

  const sendBleMessage = useCallback(
    async (device: Device, message: Record<string, unknown>) => {
      const json = JSON.stringify(message);
      const frames = encodeMessageIntoFrames(json, device.mtu);
      const label = labelForMessage(message);

      console.log(`Sending BLE message in ${frames.length} frame(s)`);

      try {
        if (frames.length > 1) {
          setTransferProgress({
            label,
            percent: 0,
          });
        }

        for (let index = 0; index < frames.length; index += 1) {
          const frame = frames[index];

          await bleManager.writeCharacteristicWithResponseForDevice(
            device.id,
            SERVICE_UUID,
            RX_CHARACTERISTIC_UUID,
            encodeBytesToBase64(frame),
          );

          const frameNumber = index + 1;

          if (frames.length > 1) {
            setTransferProgress({
              label,
              percent: Math.round((frameNumber / frames.length) * 100),
            });
          }

          if (shouldLogFrameProgress(frameNumber, frames.length)) {
            console.log(`Sent BLE frame ${frameNumber}/${frames.length}`);
          }
        }
      } finally {
        setTransferProgress(null);
      }
    },
    [],
  );

  const cleanUpConnection = useCallback(() => {
    txFrameAssemblerRef.current?.clear();

    txSubscriptionRef.current?.remove();
    txSubscriptionRef.current = null;

    disconnectSubscriptionRef.current?.remove();
    disconnectSubscriptionRef.current = null;

    setConnectedDevice(null);
    setConnectionStatus('disconnected');
  }, []);

  const subscribeToTx = useCallback((device: Device) => {
    txSubscriptionRef.current?.remove();
    txFrameAssemblerRef.current?.clear();

    txSubscriptionRef.current = bleManager.monitorCharacteristicForDevice(
      device.id,
      SERVICE_UUID,
      TX_CHARACTERISTIC_UUID,
      (error, characteristic) => {
        if (error) {
          if (
            error.errorCode === BleErrorCode.DeviceDisconnected ||
            error.errorCode === BleErrorCode.OperationCancelled
          ) {
            console.log('TX monitor stopped');
            return;
          }

          console.error('Unexpected TX monitor error:', {
            errorCode: error.errorCode,
            reason: error.reason,
            androidErrorCode: error.androidErrorCode,
          });

          return;
        }

        if (!characteristic?.value) {
          return;
        }

        try {
          const frameBytes = decodeBase64ToBytes(characteristic.value);
          const completeMessage =
            txFrameAssemblerRef.current?.acceptFrame(frameBytes);

          if (!completeMessage) {
            return;
          }

          console.log('Received complete TX message:', completeMessage);
          console.log('Parsed TX message:', JSON.parse(completeMessage));
        } catch (frameError) {
          console.error('Could not process BLE frame:', frameError);
        }
      },
    );

    console.log('Subscribed to TX characteristic');
  }, []);

  const inspectGatt = useCallback(async (device: Device) => {
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
  }, []);

  const connectToDevice = useCallback(
    async (device: Device) => {
      try {
        bleManager.stopDeviceScan();
        setIsScanning(false);
        setConnectionStatus('connecting');

        const connected = await device.connect();

        console.log('Connected to device:', connected.name, connected.id);

        const mtuDevice = await connected.requestMTU(247);

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

            cleanUpConnection();
          },
        );

        setConnectedDevice(discovered);
        setConnectionStatus('ready');
      } catch (error) {
        console.error('BLE connection error:', error);
        cleanUpConnection();
      }
    },
    [cleanUpConnection, inspectGatt, subscribeToTx],
  );

  const startScan = useCallback(async () => {
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
  }, [bleState]);

  const disconnectFromDevice = useCallback(async () => {
    if (!connectedDevice || connectionStatus === 'disconnecting') {
      return;
    }

    setConnectionStatus('disconnecting');

    try {
      await sendBleMessage(connectedDevice, createDisconnectMessage());
      console.log('Disconnect request sent; waiting for terminal');
    } catch (error) {
      console.error('BLE disconnect request error:', error);
      setConnectionStatus('ready');
    }
  }, [connectedDevice, connectionStatus, sendBleMessage]);

  const sendPing = useCallback(async () => {
    if (!connectedDevice || connectionStatus !== 'ready') {
      console.log('Not connected');
      return;
    }

    try {
      await sendBleMessage(connectedDevice, createPingMessage());
      console.log('Ping sent');
    } catch (error) {
      console.error('Error sending ping:', error);
    }
  }, [connectedDevice, connectionStatus, sendBleMessage]);

  useEffect(() => {
    const stateSubscription = bleManager.onStateChange(nextState => {
      console.log('BLE state:', nextState);
      setBleState(nextState);
    }, true);

    return () => {
      stateSubscription.remove();
      txSubscriptionRef.current?.remove();
      txFrameAssemblerRef.current?.stop();
      txFrameAssemblerRef.current = null;
    };
  }, []);

  return {
    bleState,
    isScanning,
    devices,
    connectionStatus,
    connectedDevice,
    transferProgress,
    startScan,
    connectToDevice,
    disconnectFromDevice,
    sendBleMessage,
    sendPing,
  };
}
