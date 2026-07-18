import { useCallback, useEffect, useRef, useState } from 'react';
import {
  AppState,
  NativeEventEmitter,
  NativeModules,
} from 'react-native';
import type { Device } from 'react-native-ble-plx';

import type { ConnectionStatus } from '../ble/useTerminalBle';

type SwirskiNotificationsModule = {
  addListener(eventName: string): void;
  removeListeners(count: number): void;
  isNotificationAccessEnabled(): Promise<boolean>;
  openNotificationAccessSettings(): Promise<void>;
  createSnapshotMessageJson(messageId: string): Promise<string>;
};

const SwirskiNotifications = NativeModules.SwirskiNotifications as
  | SwirskiNotificationsModule
  | undefined;

type UseNotificationBridgeArgs = {
  connectedDevice: Device | null;
  connectionStatus: ConnectionStatus;
  sendBleMessage(
    device: Device,
    message: Record<string, unknown>,
  ): Promise<void>;
};

export function useNotificationBridge({
  connectedDevice,
  connectionStatus,
  sendBleMessage,
}: UseNotificationBridgeArgs) {
  const [notificationAccessEnabled, setNotificationAccessEnabled] =
    useState<boolean>(false);

  const connectedDeviceRef = useRef<Device | null>(null);
  const connectionStatusRef = useRef<ConnectionStatus>('disconnected');
  const sendBleMessageRef = useRef(sendBleMessage);

  const refreshNotificationAccess = useCallback(async () => {
    if (!SwirskiNotifications) {
      return;
    }

    try {
      const isEnabled =
        await SwirskiNotifications.isNotificationAccessEnabled();

      setNotificationAccessEnabled(isEnabled);
    } catch (error) {
      console.error('Could not check notification access:', error);
    }
  }, []);

  const openNotificationAccessSettings = useCallback(async () => {
    if (!SwirskiNotifications) {
      return;
    }

    try {
      await SwirskiNotifications.openNotificationAccessSettings();
    } catch (error) {
      console.error('Could not open notification access settings:', error);
    }
  }, []);

  const sendCurrentNotificationSnapshot = useCallback(
    async (device: Device) => {
      if (!SwirskiNotifications) {
        console.log('Native notification module is not available');
        return;
      }

      const hasNotificationAccess =
        await SwirskiNotifications.isNotificationAccessEnabled();

      setNotificationAccessEnabled(hasNotificationAccess);

      if (!hasNotificationAccess) {
        console.log('Notification access is not enabled');
        return;
      }

      const messageId = `mobile-snapshot-${Date.now()}`;
      const snapshotJson =
        await SwirskiNotifications.createSnapshotMessageJson(messageId);

      const snapshotMessage = JSON.parse(snapshotJson) as Record<
        string,
        unknown
      >;

      await sendBleMessage(device, snapshotMessage);

      console.log('Notification snapshot sent');
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
    refreshNotificationAccess();

    const appStateSubscription = AppState.addEventListener(
      'change',
      nextAppState => {
        if (nextAppState === 'active') {
          refreshNotificationAccess();
        }
      },
    );

    const notificationEventSubscription = SwirskiNotifications
      ? new NativeEventEmitter(SwirskiNotifications).addListener(
          'SwirskiNotificationReceived',
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

              console.log('Live notification sent');
            } catch (error) {
              console.error('Could not send live notification:', error);
            }
          },
        )
      : null;

    const notificationRemovedSubscription = SwirskiNotifications
      ? new NativeEventEmitter(SwirskiNotifications).addListener(
          'SwirskiNotificationRemoved',
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
              console.log('Notification removal sent');
            } catch (error) {
              console.error('Could not send notification removal:', error);
            }
          },
        )
      : null;

    return () => {
      appStateSubscription.remove();
      notificationEventSubscription?.remove();
      notificationRemovedSubscription?.remove();
    };
  }, [refreshNotificationAccess]);

  return {
    notificationAccessEnabled,
    openNotificationAccessSettings,
    sendCurrentNotificationSnapshot,
  };
}
