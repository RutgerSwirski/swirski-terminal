import { useCallback, useEffect, useRef, useState } from 'react';
import { NativeModules } from 'react-native';
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
import {
  createDisconnectMessage,
  createPingMessage,
} from '../protocol/messages';
import { handleMusicCommandMessage } from '../music/handleMusicCommand';

type SwirskiBackgroundModule = {
  requestEnableBluetooth(): Promise<void>;
  start(deviceId: string): Promise<void>;
  stop(): Promise<void>;
  getSavedDeviceId(): Promise<string | null>;
};

const SwirskiBackground = NativeModules.SwirskiBackground as
  | SwirskiBackgroundModule
  | undefined;

const RECONNECT_DELAY_MS = 3000;

export type ConnectionStatus =
  | 'disconnected'
  | 'connecting'
  | 'discovering'
  | 'pairing'
  | 'ready'
  | 'disconnecting'
  | 'error';

export type TransferProgress = {
  label: string;
  percent: number;
} | null;

export type MessageHandler = (message: Record<string, unknown>) => void;

function shouldLogFrameProgress(
  frameIndex: number,
  frameCount: number,
): boolean {
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
  const reconnectTimerRef = useRef<ReturnType<typeof setTimeout> | null>(null);
  const reconnectRef = useRef<(deviceId: string) => void>(() => {});
  const manualDisconnectRef = useRef<boolean>(false);
  const isConnectingRef = useRef<boolean>(false);
  const restoredConnectionRef = useRef<boolean>(false);
  const messageHandlersRef = useRef<Set<MessageHandler>>(new Set());

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

  const clearReconnectTimer = useCallback(() => {
    if (reconnectTimerRef.current !== null) {
      clearTimeout(reconnectTimerRef.current);
      reconnectTimerRef.current = null;
    }
  }, []);

  const scheduleReconnect = useCallback(
    (deviceId: string) => {
      clearReconnectTimer();

      reconnectTimerRef.current = setTimeout(() => {
        reconnectTimerRef.current = null;
        reconnectRef.current(deviceId);
      }, RECONNECT_DELAY_MS);
    },
    [clearReconnectTimer],
  );

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

          const parsedMessage = JSON.parse(completeMessage) as Record<
            string,
            unknown
          >;

          console.log('Parsed TX message:', parsedMessage);

          messageHandlersRef.current.forEach(handler => {
            handler(parsedMessage);
          });

          handleMusicCommandMessage(parsedMessage).catch(commandError => {
            console.error('Could not handle music command:', commandError);
          });
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

  const prepareConnectedDevice = useCallback(
    async (connected: Device) => {
      const mtuDevice = await connected.requestMTU(247);

      console.log('Negotiated MTU:', mtuDevice.mtu);

      setConnectionStatus('discovering');

      const discovered =
        await mtuDevice.discoverAllServicesAndCharacteristics();

      await inspectGatt(discovered);
      setConnectionStatus('pairing');
      await bleManager.readCharacteristicForDevice(
        discovered.id,
        SERVICE_UUID,
        TX_CHARACTERISTIC_UUID,
      );
      subscribeToTx(discovered);

      disconnectSubscriptionRef.current?.remove();

      disconnectSubscriptionRef.current = bleManager.onDeviceDisconnected(
        discovered.id,
        error => {
          if (error) {
            console.log('BLE disconnected:', error);
          }

          const shouldReconnect = !manualDisconnectRef.current;
          cleanUpConnection();

          if (shouldReconnect) {
            scheduleReconnect(discovered.id);
          }
        },
      );

      manualDisconnectRef.current = false;
      setConnectedDevice(discovered);
      setConnectionStatus('ready');

      try {
        await SwirskiBackground?.start(discovered.id);
      } catch (error) {
        console.error('Could not start background connection service:', error);
      }
    },
    [cleanUpConnection, inspectGatt, scheduleReconnect, subscribeToTx],
  );

  const reconnectToDevice = useCallback(
    async (deviceId: string) => {
      if (isConnectingRef.current || manualDisconnectRef.current) {
        return;
      }

      isConnectingRef.current = true;

      try {
        setConnectionStatus('connecting');

        const connected = await bleManager.connectToDevice(deviceId);

        console.log('Reconnected to device:', connected.name, connected.id);
        await prepareConnectedDevice(connected);
      } catch (error) {
        console.log('BLE reconnect failed; retrying:', error);
        cleanUpConnection();
        scheduleReconnect(deviceId);
      } finally {
        isConnectingRef.current = false;
      }
    },
    [cleanUpConnection, prepareConnectedDevice, scheduleReconnect],
  );

  reconnectRef.current = deviceId => {
    reconnectToDevice(deviceId).catch(error => {
      console.error('Could not reconnect to terminal:', error);
    });
  };

  const connectToDevice = useCallback(
    async (device: Device) => {
      if (isConnectingRef.current) {
        return;
      }

      isConnectingRef.current = true;
      manualDisconnectRef.current = false;
      clearReconnectTimer();

      try {
        bleManager.stopDeviceScan();
        setIsScanning(false);
        setConnectionStatus('connecting');

        const connected = await device.connect();

        console.log('Connected to device:', connected.name, connected.id);
        await prepareConnectedDevice(connected);
      } catch (error) {
        console.error('BLE connection error:', error);
        cleanUpConnection();
      } finally {
        isConnectingRef.current = false;
      }
    },
    [clearReconnectTimer, cleanUpConnection, prepareConnectedDevice],
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

    bleManager.startDeviceScan([SERVICE_UUID], null, (error, device) => {
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
        const deviceName = device.name ?? device.localName;
        const existingIndex = currentDevices.findIndex(currentDevice => {
          const currentName = currentDevice.name ?? currentDevice.localName;

          return (
            currentDevice.id === device.id ||
            (deviceName !== null && deviceName === currentName)
          );
        });

        if (existingIndex >= 0) {
          const updatedDevices = [...currentDevices];
          updatedDevices[existingIndex] = device;
          return updatedDevices;
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
    manualDisconnectRef.current = true;
    clearReconnectTimer();

    try {
      await sendBleMessage(connectedDevice, createDisconnectMessage());
      await SwirskiBackground?.stop();
      console.log('Disconnect request sent; waiting for terminal');
    } catch (error) {
      console.error('BLE disconnect request error:', error);
      manualDisconnectRef.current = false;
      setConnectionStatus('ready');
    }
  }, [clearReconnectTimer, connectedDevice, connectionStatus, sendBleMessage]);

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

  const enableBluetooth = useCallback(async () => {
    try {
      await SwirskiBackground?.requestEnableBluetooth();
    } catch (error) {
      console.error('Could not enable Bluetooth:', error);
    }
  }, []);

  const addMessageHandler = useCallback((handler: MessageHandler) => {
    messageHandlersRef.current.add(handler);

    return () => {
      messageHandlersRef.current.delete(handler);
    };
  }, []);

  useEffect(() => {
    const stateSubscription = bleManager.onStateChange(nextState => {
      console.log('BLE state:', nextState);
      setBleState(nextState);

      if (nextState !== State.PoweredOn) {
        bleManager.stopDeviceScan();
        setDevices([]);
        setIsScanning(false);
      }
    }, true);

    return () => {
      stateSubscription.remove();
      clearReconnectTimer();
      txSubscriptionRef.current?.remove();
      txFrameAssemblerRef.current?.stop();
      txFrameAssemblerRef.current = null;
    };
  }, [clearReconnectTimer]);

  useEffect(() => {
    if (bleState !== State.PoweredOn || restoredConnectionRef.current) {
      return;
    }

    restoredConnectionRef.current = true;

    async function restoreConnection() {
      try {
        const deviceId = await SwirskiBackground?.getSavedDeviceId();

        if (deviceId) {
          reconnectRef.current(deviceId);
        }
      } catch (error) {
        console.error('Could not restore terminal connection:', error);
      }
    }

    restoreConnection().catch(error => {
      console.error('Could not restore terminal connection:', error);
    });
  }, [bleState]);

  return {
    bleState,
    isScanning,
    devices,
    connectionStatus,
    connectedDevice,
    transferProgress,
    addMessageHandler,
    enableBluetooth,
    startScan,
    connectToDevice,
    disconnectFromDevice,
    sendBleMessage,
    sendPing,
  };
}
