import { useCallback, useEffect, useRef } from 'react';
import { NativeEventEmitter, NativeModules } from 'react-native';
import type { Device } from 'react-native-ble-plx';

import type { ConnectionStatus } from '../ble/useTerminalBle';

type SwirskiMediaModule = {
  addListener(eventName: string): void;
  removeListeners(count: number): void;
  getCurrentMusicStateMessageJson(messageId: string): Promise<string | null>;
};

const SwirskiMedia = NativeModules.SwirskiMedia as
  | SwirskiMediaModule
  | undefined;

type UseMusicBridgeArgs = {
  connectedDevice: Device | null;
  connectionStatus: ConnectionStatus;
  sendBleMessage(
    device: Device,
    message: Record<string, unknown>,
  ): Promise<void>;
};

export function useMusicBridge({
  connectedDevice,
  connectionStatus,
  sendBleMessage,
}: UseMusicBridgeArgs) {
  const connectedDeviceRef = useRef<Device | null>(null);
  const connectionStatusRef = useRef<ConnectionStatus>('disconnected');
  const sendBleMessageRef = useRef(sendBleMessage);

  const sendCurrentMusicState = useCallback(
    async (device: Device) => {
      if (!SwirskiMedia) {
        console.log('Native media module is not available');
        return;
      }

      const messageId = `mobile-music-${Date.now()}`;
      const messageJson =
        await SwirskiMedia.getCurrentMusicStateMessageJson(messageId);

      if (!messageJson) {
        console.log('No active music session found');
        return;
      }

      const message = JSON.parse(messageJson) as Record<string, unknown>;

      await sendBleMessage(device, message);

      console.log('Current music state sent');
    },
    [sendBleMessage],
  );

  useEffect(() => {
    connectedDeviceRef.current = connectedDevice;
  }, [connectedDevice]);

  useEffect(() => {
    connectionStatusRef.current = connectionStatus;
  }, [connectionStatus]);

  useEffect(() => {
    sendBleMessageRef.current = sendBleMessage;
  }, [sendBleMessage]);

  useEffect(() => {
    const musicEventSubscription = SwirskiMedia
      ? new NativeEventEmitter(SwirskiMedia).addListener(
          'SwirskiMusicStateChanged',
          async (messageJson: string) => {
            const device = connectedDeviceRef.current;

            if (!device || connectionStatusRef.current !== 'ready') {
              return;
            }

            try {
              const message = JSON.parse(messageJson) as Record<
                string,
                unknown
              >;

              await sendBleMessageRef.current(device, message);

              console.log('Live music state sent');
            } catch (error) {
              console.error('Could not send live music state:', error);
            }
          },
        )
      : null;

    return () => {
      musicEventSubscription?.remove();
    };
  }, []);

  return {
    sendCurrentMusicState,
  };
}
