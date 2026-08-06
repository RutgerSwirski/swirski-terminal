import { useCallback, useEffect, useRef, useState } from 'react';
import { AppState, NativeModules } from 'react-native';
import { BleErrorCode, ScanMode, State } from 'react-native-ble-plx';
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
  start(deviceId: string, connected: boolean): Promise<void>;
  stop(): Promise<void>;
  getSavedDeviceId(): Promise<string | null>;
};

const SwirskiBackground = NativeModules.SwirskiBackground as
  | SwirskiBackgroundModule
  | undefined;

const RECONNECT_DELAY_MS = 3000;
const RECONNECT_SCAN_TIMEOUT_MS = 10000;
const CONNECT_TIMEOUT_MS = 10000;
const CONNECTION_WATCHDOG_INTERVAL_MS = 5000;
const TERMINAL_NAME = 'Swirski Terminal';

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
  const scanTimerRef = useRef<ReturnType<typeof setTimeout> | null>(null);
  const reconnectScanCancelRef = useRef<(() => void) | null>(null);
  const reconnectTimerRef = useRef<ReturnType<typeof setTimeout> | null>(null);
  const reconnectRef = useRef<(deviceId: string) => void>(() => {});
  const bleStateRef = useRef<State>(State.Unknown);
  const disposedRef = useRef<boolean>(false);
  const manualDisconnectRef = useRef<boolean>(false);
  const isConnectingRef = useRef<boolean>(false);
  const isCheckingConnectionRef = useRef<boolean>(false);
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

  const stopScan = useCallback(() => {
    const cancelReconnectScan = reconnectScanCancelRef.current;
    reconnectScanCancelRef.current = null;

    if (scanTimerRef.current !== null) {
      clearTimeout(scanTimerRef.current);
      scanTimerRef.current = null;
    }

    bleManager.stopDeviceScan().catch(error => {
      console.log('Could not stop BLE scan:', error);
    });
    setIsScanning(false);

    cancelReconnectScan?.();
  }, []);

  const scheduleReconnect = useCallback(
    (deviceId: string) => {
      if (
        disposedRef.current ||
        manualDisconnectRef.current ||
        bleStateRef.current !== State.PoweredOn
      ) {
        return;
      }

      clearReconnectTimer();

      SwirskiBackground?.start(deviceId, false).catch(error => {
        console.error('Could not update background connection status:', error);
      });

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
      disconnectSubscriptionRef.current?.remove();

      disconnectSubscriptionRef.current = bleManager.onDeviceDisconnected(
        connected.id,
        error => {
          if (error) {
            console.log('BLE disconnected:', error);
          }

          const shouldReconnect = !manualDisconnectRef.current;
          cleanUpConnection();

          if (shouldReconnect) {
            scheduleReconnect(connected.id);
          }
        },
      );

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

      manualDisconnectRef.current = false;
      setConnectedDevice(discovered);
      setConnectionStatus('ready');

      try {
        await SwirskiBackground?.start(discovered.id, true);
      } catch (error) {
        console.error('Could not start background connection service:', error);
      }
    },
    [cleanUpConnection, inspectGatt, scheduleReconnect, subscribeToTx],
  );

  const scanForReconnectDevice = useCallback(
    (savedDeviceId: string): Promise<Device> => {
      stopScan();
      setIsScanning(true);

      return new Promise((resolve, reject) => {
        let settled = false;

        const finish = (device?: Device, error?: unknown) => {
          if (settled) {
            return;
          }

          settled = true;
          reconnectScanCancelRef.current = null;
          stopScan();

          if (device) {
            resolve(device);
          } else {
            reject(error ?? new Error('Terminal was not found'));
          }
        };

        scanTimerRef.current = setTimeout(() => {
          finish(undefined, new Error('BLE reconnect scan timed out'));
        }, RECONNECT_SCAN_TIMEOUT_MS);

        reconnectScanCancelRef.current = () => {
          finish(undefined, new Error('BLE reconnect scan was cancelled'));
        };

        bleManager
          .startDeviceScan(
            [SERVICE_UUID],
            { scanMode: ScanMode.LowLatency },
            (error, device) => {
              if (error) {
                finish(undefined, error);
                return;
              }

              if (!device) {
                return;
              }

              const deviceName = device.name ?? device.localName;

              if (
                device.id === savedDeviceId ||
                deviceName === TERMINAL_NAME
              ) {
                console.log(
                  'Found terminal for reconnect:',
                  deviceName,
                  device.id,
                );
                finish(device);
              }
            },
          )
          .catch(error => finish(undefined, error));
      });
    },
    [stopScan],
  );

  const reconnectToDevice = useCallback(
    async (deviceId: string) => {
      if (isConnectingRef.current || manualDisconnectRef.current) {
        return;
      }

      isConnectingRef.current = true;
      let attemptedDeviceId = deviceId;

      try {
        setConnectionStatus('connecting');

        let isStillConnected = false;

        try {
          isStillConnected = await bleManager.isDeviceConnected(deviceId);
        } catch (error) {
          console.log(
            'Saved BLE device is not known to this manager; scanning:',
            error,
          );
        }

        if (isStillConnected) {
          const knownDevices = await bleManager.devices([deviceId]);
          const knownDevice = knownDevices[0];

          if (knownDevice) {
            console.log('Restored existing BLE connection:', knownDevice.id);
            await prepareConnectedDevice(knownDevice);
            return;
          }
        }

        const reconnectDevice = await scanForReconnectDevice(deviceId);
        attemptedDeviceId = reconnectDevice.id;
        const connected = await reconnectDevice.connect({
          autoConnect: false,
          timeout: CONNECT_TIMEOUT_MS,
        });

        console.log('Reconnected to device:', connected.name, connected.id);
        await prepareConnectedDevice(connected);
      } catch (error) {
        console.log('BLE reconnect failed; retrying:', error);
        cleanUpConnection();

        try {
          await bleManager.cancelDeviceConnection(attemptedDeviceId);
        } catch (cancelError) {
          console.log('Could not close failed BLE connection:', cancelError);
        }

        if (!manualDisconnectRef.current) {
          scheduleReconnect(deviceId);
        }
      } finally {
        isConnectingRef.current = false;
      }
    },
    [
      cleanUpConnection,
      prepareConnectedDevice,
      scanForReconnectDevice,
      scheduleReconnect,
    ],
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
        stopScan();
        setConnectionStatus('connecting');

        const connected = await device.connect({ timeout: CONNECT_TIMEOUT_MS });

        console.log('Connected to device:', connected.name, connected.id);
        await prepareConnectedDevice(connected);
      } catch (error) {
        console.error('BLE connection error:', error);
        cleanUpConnection();

        try {
          await bleManager.cancelDeviceConnection(device.id);
        } catch (cancelError) {
          console.log('Could not close failed BLE connection:', cancelError);
        }
      } finally {
        isConnectingRef.current = false;
      }
    },
    [
      clearReconnectTimer,
      cleanUpConnection,
      prepareConnectedDevice,
      stopScan,
    ],
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

    stopScan();
    setDevices([]);
    setIsScanning(true);

    bleManager.startDeviceScan([SERVICE_UUID], null, (error, device) => {
      if (error) {
        console.error('BLE scan error:', error);
        stopScan();
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

    scanTimerRef.current = setTimeout(stopScan, 5000);
  }, [bleState, stopScan]);

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
    disposedRef.current = false;

    const stateSubscription = bleManager.onStateChange(nextState => {
      console.log('BLE state:', nextState);
      bleStateRef.current = nextState;
      setBleState(nextState);

      if (nextState !== State.PoweredOn) {
        stopScan();
        clearReconnectTimer();
        restoredConnectionRef.current = false;
        setDevices([]);
        cleanUpConnection();
      }
    }, true);

    return () => {
      disposedRef.current = true;
      stateSubscription.remove();
      clearReconnectTimer();
      stopScan();
      txSubscriptionRef.current?.remove();
      txFrameAssemblerRef.current?.stop();
      txFrameAssemblerRef.current = null;
    };
  }, [cleanUpConnection, clearReconnectTimer, stopScan]);

  useEffect(() => {
    if (bleState !== State.PoweredOn || restoredConnectionRef.current) {
      return;
    }

    restoredConnectionRef.current = true;

    async function restoreConnection() {
      try {
        const hasPermission = await requestBlePermissions();

        if (!hasPermission) {
          console.log('BLE permission denied; auto reconnect is disabled');
          return;
        }

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

  useEffect(() => {
    if (!connectedDevice || connectionStatus !== 'ready') {
      return;
    }

    const deviceId = connectedDevice.id;

    async function verifyConnection() {
      if (
        isCheckingConnectionRef.current ||
        isConnectingRef.current ||
        manualDisconnectRef.current
      ) {
        return;
      }

      isCheckingConnectionRef.current = true;

      try {
        const isConnected = await bleManager.isDeviceConnected(deviceId);

        if (isConnected || manualDisconnectRef.current) {
          return;
        }

        console.log('BLE watchdog detected a stale connection');
        cleanUpConnection();

        try {
          await bleManager.cancelDeviceConnection(deviceId);
        } catch (error) {
          console.log('Could not close stale BLE connection:', error);
        }

        if (!manualDisconnectRef.current) {
          scheduleReconnect(deviceId);
        }
      } catch (error) {
        console.log('Could not verify BLE connection:', error);
      } finally {
        isCheckingConnectionRef.current = false;
      }
    }

    const watchdog = setInterval(() => {
      verifyConnection().catch(error => {
        console.error('BLE connection watchdog failed:', error);
      });
    }, CONNECTION_WATCHDOG_INTERVAL_MS);

    const appStateSubscription = AppState.addEventListener(
      'change',
      nextState => {
        if (nextState === 'active') {
          verifyConnection().catch(error => {
            console.error('Could not verify resumed BLE connection:', error);
          });
        }
      },
    );

    return () => {
      clearInterval(watchdog);
      appStateSubscription.remove();
    };
  }, [
    cleanUpConnection,
    connectedDevice,
    connectionStatus,
    scheduleReconnect,
  ]);

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
