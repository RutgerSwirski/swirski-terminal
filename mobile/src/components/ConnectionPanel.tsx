import React from 'react';
import { Button, StyleSheet, Text, View } from 'react-native';
import type { Device } from 'react-native-ble-plx';
import { State } from 'react-native-ble-plx';

import type { ConnectionStatus } from '../ble/useTerminalBle';

type ConnectionPanelProps = {
  bleState: State;
  connectionStatus: ConnectionStatus;
  connectedDevice: Device | null;
  isScanning: boolean;
  notificationAccessEnabled: boolean;
  onOpenNotificationSettings(): void;
  onStartScan(): void;
};

export function ConnectionPanel({
  bleState,
  connectionStatus,
  connectedDevice,
  isScanning,
  notificationAccessEnabled,
  onOpenNotificationSettings,
  onStartScan,
}: ConnectionPanelProps) {
  return (
    <>
      <Text>Swirski Terminal</Text>
      <Text>Bluetooth: {bleState}</Text>
      <Text>Status: {connectionStatus}</Text>

      {!notificationAccessEnabled && (
        <View style={styles.notice}>
          <Text style={styles.noticeText}>
            Enable notification access to sync phone notifications.
          </Text>

          <Button
            title="Open notification settings"
            onPress={onOpenNotificationSettings}
          />
        </View>
      )}

      {connectedDevice && <Text>Connected: {connectedDevice.name}</Text>}

      <Button
        title={isScanning ? 'Scanning...' : 'Start scanning'}
        disabled={isScanning}
        onPress={onStartScan}
      />
    </>
  );
}

const styles = StyleSheet.create({
  notice: {
    width: '90%',
    padding: 12,
    marginVertical: 12,
    borderWidth: 1,
    borderColor: '#cccccc',
  },
  noticeText: {
    marginBottom: 8,
    textAlign: 'center',
  },
});
