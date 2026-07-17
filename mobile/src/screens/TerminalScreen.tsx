import React, { useCallback, useEffect, useRef } from 'react';
import { ScrollView, StyleSheet, View } from 'react-native';
import type { Device } from 'react-native-ble-plx';
import { useSafeAreaInsets } from 'react-native-safe-area-context';

import { useTerminalBle } from '../ble/useTerminalBle';
import { ConnectionPanel } from '../components/ConnectionPanel';
import { DebugActions } from '../components/DebugActions';
import { DeviceList } from '../components/DeviceList';
import { useNotificationBridge } from '../notifications/useNotificationBridge';
import {
  createTestNotificationReceivedMessage,
  createTestNotificationSnapshotMessage,
  createTimeSyncMessage,
} from '../protocol/messages';

export function TerminalScreen() {
  const terminalBle = useTerminalBle();
  const insets = useSafeAreaInsets();
  const lastSyncedDeviceIdRef = useRef<string | null>(null);

  const notificationBridge = useNotificationBridge({
    connectedDevice: terminalBle.connectedDevice,
    connectionStatus: terminalBle.connectionStatus,
    sendBleMessage: terminalBle.sendBleMessage,
  });

  const sendTestNotificationSnapshot = useCallback(
    async (device: Device) => {
      await terminalBle.sendBleMessage(
        device,
        createTestNotificationSnapshotMessage(),
      );
    },
    [terminalBle],
  );

  const sendTestNotificationReceived = useCallback(
    async (device: Device) => {
      await terminalBle.sendBleMessage(
        device,
        createTestNotificationReceivedMessage(),
      );
    },
    [terminalBle],
  );

  useEffect(() => {
    const device = terminalBle.connectedDevice;

    if (!device || terminalBle.connectionStatus !== 'ready') {
      lastSyncedDeviceIdRef.current = null;
      return;
    }

    if (lastSyncedDeviceIdRef.current === device.id) {
      return;
    }

    lastSyncedDeviceIdRef.current = device.id;

    async function sendInitialState() {
      if (!device) {
        return;
      }

      try {
        await terminalBle.sendBleMessage(device, createTimeSyncMessage());
      } catch (error) {
        console.error('Could not send date/time sync:', error);
      }

      try {
        await notificationBridge.sendCurrentNotificationSnapshot(device);
      } catch (error) {
        console.error('Could not send notification snapshot:', error);
      }
    }

    sendInitialState();
  }, [notificationBridge, terminalBle]);

  return (
    <ScrollView
      contentContainerStyle={[
        styles.container,
        {
          paddingTop: insets.top + 20,
          paddingBottom: insets.bottom + 20,
        },
      ]}
    >
      <View style={styles.content}>
        <ConnectionPanel
          bleState={terminalBle.bleState}
          connectionStatus={terminalBle.connectionStatus}
          connectedDevice={terminalBle.connectedDevice}
          isScanning={terminalBle.isScanning}
          notificationAccessEnabled={
            notificationBridge.notificationAccessEnabled
          }
          onOpenNotificationSettings={
            notificationBridge.openNotificationAccessSettings
          }
          onStartScan={terminalBle.startScan}
        />

        <DeviceList
          devices={terminalBle.devices}
          connectedDevice={terminalBle.connectedDevice}
          connectionStatus={terminalBle.connectionStatus}
          onConnect={terminalBle.connectToDevice}
          onDisconnect={terminalBle.disconnectFromDevice}
        />

        <DebugActions
          connectedDevice={terminalBle.connectedDevice}
          connectionStatus={terminalBle.connectionStatus}
          onPing={terminalBle.sendPing}
          onSendTestNotificationSnapshot={sendTestNotificationSnapshot}
          onSendTestNotificationReceived={sendTestNotificationReceived}
        />
      </View>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: {
    flexGrow: 1,
    backgroundColor: '#f6f2e8',
    paddingHorizontal: 20,
    alignItems: 'center',
    justifyContent: 'flex-start',
  },
  content: {
    width: '100%',
    maxWidth: 520,
    gap: 16,
  },
});
