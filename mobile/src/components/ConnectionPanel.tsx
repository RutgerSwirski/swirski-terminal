import React from 'react';
import { StyleSheet, View } from 'react-native';
import { Button, Card, CardContent, Text, Title } from '@swirski/ui/native';
import type { Device } from 'react-native-ble-plx';
import { State } from 'react-native-ble-plx';

import type { ConnectionStatus } from '../ble/useTerminalBle';

type ConnectionPanelProps = {
  bleState: State;
  connectionStatus: ConnectionStatus;
  connectedDevice: Device | null;
  isScanning: boolean;
  notificationAccessEnabled: boolean;
  onEnableBluetooth(): void;
  onOpenNotificationSettings(): void;
  onStartScan(): void;
};

export function ConnectionPanel({
  bleState,
  connectionStatus,
  connectedDevice,
  isScanning,
  notificationAccessEnabled,
  onEnableBluetooth,
  onOpenNotificationSettings,
  onStartScan,
}: ConnectionPanelProps) {
  return (
    <>
      <View style={styles.header}>
        <Title size="h2">Swirski Terminal</Title>
        <Text tone="muted">Bluetooth: {bleState}</Text>
        <Text tone="muted">Status: {connectionStatus}</Text>
      </View>

      {!notificationAccessEnabled && (
        <Card variant="outline" tone="yellow" style={styles.notice}>
          <CardContent>
            <Text style={styles.noticeText}>
              Enable notification access to sync phone notifications.
            </Text>

            <Button tone="black" onPress={onOpenNotificationSettings}>
              Open notification settings
            </Button>
          </CardContent>
        </Card>
      )}

      {bleState === State.PoweredOn && connectedDevice && (
        <Text weight="medium">
          Connected:{' '}
          {connectedDevice.name ??
            connectedDevice.localName ??
            'Swirski Terminal'}
        </Text>
      )}

      {bleState === State.PoweredOff && (
        <Button onPress={onEnableBluetooth}>Turn on Bluetooth</Button>
      )}

      {bleState === State.PoweredOn && !connectedDevice && (
        <Button
          disabled={isScanning}
          onPress={onStartScan}
        >
          {isScanning ? 'Scanning...' : 'Start scanning'}
        </Button>
      )}
    </>
  );
}

const styles = StyleSheet.create({
  header: {
    alignItems: 'center',
    gap: 4,
  },
  notice: {
    width: '90%',
    marginVertical: 12,
  },
  noticeText: {
    marginBottom: 8,
    textAlign: 'center',
  },
});
