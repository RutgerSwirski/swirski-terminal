import { useCallback, useEffect, useState } from 'react';
import type { Device } from 'react-native-ble-plx';

import type { ConnectionStatus, MessageHandler } from '../ble/useTerminalBle';
import {
  createWifiConfigureMessage,
  createWifiDisconnectMessage,
  createWifiInternetTestMessage,
  createWifiScanRequestMessage,
} from '../protocol/messages';

export type TerminalWifiNetwork = {
  ssid: string;
  signalStrength: number;
  secured: boolean;
};

export type TerminalWifiStatus = {
  state: string;
  ssid: string;
  scanning: boolean;
  internetTest: string;
  internetLatencyMs: number;
};

type UseTerminalWifiOptions = {
  connectedDevice: Device | null;
  connectionStatus: ConnectionStatus;
  addMessageHandler(handler: MessageHandler): () => void;
  sendBleMessage(
    device: Device,
    message: Record<string, unknown>,
  ): Promise<void>;
};

export function useTerminalWifi({
  connectedDevice,
  connectionStatus,
  addMessageHandler,
  sendBleMessage,
}: UseTerminalWifiOptions) {
  const [networks, setNetworks] = useState<TerminalWifiNetwork[]>([]);
  const [status, setStatus] = useState<TerminalWifiStatus>({
    state: 'disconnected',
    ssid: '',
    scanning: false,
    internetTest: 'idle',
    internetLatencyMs: 0,
  });

  useEffect(() => {
    if (connectedDevice && connectionStatus === 'ready') {
      return;
    }

    setNetworks([]);
    setStatus({
      state: 'disconnected',
      ssid: '',
      scanning: false,
      internetTest: 'idle',
      internetLatencyMs: 0,
    });
  }, [connectedDevice, connectionStatus]);

  useEffect(() => {
    return addMessageHandler(message => {
      if (typeof message.payload !== 'object' || message.payload === null) {
        return;
      }

      const payload = message.payload as Record<string, unknown>;

      if (message.type === 'wifi.networks' && Array.isArray(payload.networks)) {
        setNetworks(payload.networks as TerminalWifiNetwork[]);
      }

      if (message.type === 'wifi.status' && typeof payload.state === 'string') {
        setStatus({
          state: payload.state,
          ssid: typeof payload.ssid === 'string' ? payload.ssid : '',
          scanning: payload.scanning === true,
          internetTest:
            typeof payload.internetTest === 'string'
              ? payload.internetTest
              : 'idle',
          internetLatencyMs:
            typeof payload.internetLatencyMs === 'number'
              ? payload.internetLatencyMs
              : 0,
        });
      }
    });
  }, [addMessageHandler]);

  const scan = useCallback(async () => {
    if (!connectedDevice || connectionStatus !== 'ready') {
      return;
    }

    setStatus(current => ({ ...current, scanning: true }));

    try {
      await sendBleMessage(connectedDevice, createWifiScanRequestMessage());
    } catch (error) {
      console.error('Could not scan terminal Wi-Fi:', error);
      setStatus(current => ({ ...current, scanning: false }));
    }
  }, [connectedDevice, connectionStatus, sendBleMessage]);

  const connect = useCallback(
    async (ssid: string, password: string) => {
      if (!connectedDevice || connectionStatus !== 'ready') {
        return;
      }

      setStatus(current => ({ ...current, state: 'connecting' }));

      try {
        await sendBleMessage(
          connectedDevice,
          createWifiConfigureMessage(ssid, password),
        );
      } catch (error) {
        console.error('Could not configure terminal Wi-Fi:', error);
        setStatus(current => ({ ...current, state: 'failed' }));
      }
    },
    [connectedDevice, connectionStatus, sendBleMessage],
  );

  const testInternet = useCallback(async () => {
    if (!connectedDevice || connectionStatus !== 'ready') {
      return;
    }

    setStatus(current => ({ ...current, internetTest: 'testing' }));

    try {
      await sendBleMessage(connectedDevice, createWifiInternetTestMessage());
    } catch (error) {
      console.error('Could not test terminal internet connection:', error);
      setStatus(current => ({ ...current, internetTest: 'failed' }));
    }
  }, [connectedDevice, connectionStatus, sendBleMessage]);

  const disconnect = useCallback(async () => {
    if (!connectedDevice || connectionStatus !== 'ready') {
      return;
    }

    try {
      await sendBleMessage(connectedDevice, createWifiDisconnectMessage());
    } catch (error) {
      console.error('Could not disconnect terminal Wi-Fi:', error);
    }
  }, [connectedDevice, connectionStatus, sendBleMessage]);

  return { networks, status, scan, connect, testInternet, disconnect };
}
