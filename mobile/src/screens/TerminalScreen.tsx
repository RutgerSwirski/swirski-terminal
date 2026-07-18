import React, { useCallback, useEffect, useRef } from 'react';
import { ScrollView, StyleSheet, View } from 'react-native';
import { Card, CardContent, Text } from '@swirski/ui/native';
import type { Device } from 'react-native-ble-plx';
import { State } from 'react-native-ble-plx';
import { useSafeAreaInsets } from 'react-native-safe-area-context';

import { useTerminalBle } from '../ble/useTerminalBle';
import { ConnectionPanel } from '../components/ConnectionPanel';
import { DebugActions } from '../components/DebugActions';
import { DeviceList } from '../components/DeviceList';
import { WifiPanel } from '../components/WifiPanel';
import { useMusicBridge } from '../music/useMusicBridge';
import { useNotificationBridge } from '../notifications/useNotificationBridge';
import {
  createTestMusicStateMessage,
  createTestNotificationReceivedMessage,
  createTestNotificationSnapshotMessage,
  createTimeSyncMessage,
} from '../protocol/messages';
import { useTerminalWifi } from '../wifi/useTerminalWifi';

export function TerminalScreen() {
  const terminalBle = useTerminalBle();
  const connectedDevice = terminalBle.connectedDevice;
  const connectionStatus = terminalBle.connectionStatus;
  const sendBleMessage = terminalBle.sendBleMessage;
  const insets = useSafeAreaInsets();
  const lastSyncedDeviceIdRef = useRef<string | null>(null);

  const notificationBridge = useNotificationBridge({
    connectedDevice: terminalBle.connectedDevice,
    connectionStatus: terminalBle.connectionStatus,
    sendBleMessage: terminalBle.sendBleMessage,
  });

  const musicBridge = useMusicBridge({
    connectedDevice: terminalBle.connectedDevice,
    connectionStatus: terminalBle.connectionStatus,
    sendBleMessage: terminalBle.sendBleMessage,
  });
  const terminalWifi = useTerminalWifi({
    connectedDevice,
    connectionStatus,
    addMessageHandler: terminalBle.addMessageHandler,
    sendBleMessage,
  });
  const sendCurrentNotificationSnapshot =
    notificationBridge.sendCurrentNotificationSnapshot;
  const sendCurrentMusicState = musicBridge.sendCurrentMusicState;
  const scanTerminalWifi = terminalWifi.scan;

  const sendTestNotificationSnapshot = useCallback(
    async (device: Device) => {
      await sendBleMessage(device, createTestNotificationSnapshotMessage());
    },
    [sendBleMessage],
  );

  const sendTestNotificationReceived = useCallback(
    async (device: Device) => {
      await sendBleMessage(device, createTestNotificationReceivedMessage());
    },
    [sendBleMessage],
  );

  const sendTestMusicState = useCallback(
    async (device: Device) => {
      await sendBleMessage(device, createTestMusicStateMessage());
    },
    [sendBleMessage],
  );

  useEffect(() => {
    const device = connectedDevice;

    if (!device || connectionStatus !== 'ready') {
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
        await sendBleMessage(device, createTimeSyncMessage());
      } catch (error) {
        console.error('Could not send date/time sync:', error);
      }

      try {
        await scanTerminalWifi();
      } catch (error) {
        console.error('Could not request terminal Wi-Fi status:', error);
      }

      try {
        await sendCurrentNotificationSnapshot(device);
      } catch (error) {
        console.error('Could not send notification snapshot:', error);
      }

      try {
        await sendCurrentMusicState(device);
      } catch (error) {
        console.error('Could not send current music state:', error);
      }
    }

    sendInitialState();
  }, [
    connectedDevice,
    connectionStatus,
    sendBleMessage,
    sendCurrentMusicState,
    sendCurrentNotificationSnapshot,
    scanTerminalWifi,
  ]);

  useEffect(() => {
    const device = connectedDevice;

    if (!device || connectionStatus !== 'ready') {
      return;
    }

    const timeSyncInterval = setInterval(() => {
      sendBleMessage(device, createTimeSyncMessage()).catch(error => {
        console.error('Could not refresh date/time sync:', error);
      });
    }, 30 * 60 * 1000);

    return () => clearInterval(timeSyncInterval);
  }, [connectedDevice, connectionStatus, sendBleMessage]);

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
          onEnableBluetooth={terminalBle.enableBluetooth}
          onOpenNotificationSettings={
            notificationBridge.openNotificationAccessSettings
          }
          onStartScan={terminalBle.startScan}
        />

        {terminalBle.transferProgress && (
          <Card variant="outline" tone="yellow" style={styles.syncCard}>
            <CardContent style={styles.syncContent}>
              <Text weight="bold">{terminalBle.transferProgress.label}</Text>
              <Text tone="muted">{terminalBle.transferProgress.percent}%</Text>
            </CardContent>
          </Card>
        )}

        {terminalBle.bleState === State.PoweredOn && (
          <DeviceList
            devices={terminalBle.devices}
            connectedDevice={terminalBle.connectedDevice}
            connectionStatus={terminalBle.connectionStatus}
            onConnect={terminalBle.connectToDevice}
            onDisconnect={terminalBle.disconnectFromDevice}
          />
        )}

        {terminalBle.connectedDevice &&
          terminalBle.connectionStatus === 'ready' && (
            <WifiPanel
              networks={terminalWifi.networks}
              status={terminalWifi.status}
              onScan={terminalWifi.scan}
              onConnect={terminalWifi.connect}
              onTestInternet={terminalWifi.testInternet}
              onDisconnect={terminalWifi.disconnect}
            />
          )}

        <DebugActions
          connectedDevice={terminalBle.connectedDevice}
          connectionStatus={terminalBle.connectionStatus}
          onPing={terminalBle.sendPing}
          onSendTestNotificationSnapshot={sendTestNotificationSnapshot}
          onSendTestNotificationReceived={sendTestNotificationReceived}
          onSendTestMusicState={sendTestMusicState}
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
  syncCard: {
    width: '100%',
  },
  syncContent: {
    alignItems: 'center',
    gap: 4,
  },
});
