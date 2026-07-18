import React from 'react';
import { StyleSheet, View } from 'react-native';
import {
  Button,
  Card,
  CardContent,
  CardMeta,
  CardTitle,
  Text,
} from '@swirski/ui/native';
import type { Device } from 'react-native-ble-plx';

import type { ConnectionStatus } from '../ble/useTerminalBle';

type DeviceListProps = {
  devices: Device[];
  connectedDevice: Device | null;
  connectionStatus: ConnectionStatus;
  onConnect(device: Device): void;
  onDisconnect(): void;
};

export function DeviceList({
  devices,
  connectedDevice,
  connectionStatus,
  onConnect,
  onDisconnect,
}: DeviceListProps) {
  const isDeviceConnected = connectedDevice !== null;

  //if device connected filter out disconnected devices
  if (isDeviceConnected) {
    devices = devices.filter(device => device.id === connectedDevice?.id);
  }

  return (
    <View style={styles.list}>
      {devices.map(device => (
        <Card key={device.id} variant="outline" style={styles.deviceCard}>
          <CardContent>
            <CardTitle size="sm">
              {device.name ?? device.localName ?? 'Unnamed device'}
            </CardTitle>
            <CardMeta>{device.id}</CardMeta>
            <Text tone="muted">RSSI: {device.rssi ?? 'Unknown'}</Text>

            <View style={styles.actions}>
              {connectedDevice?.id === device.id ? (
                <Button
                  tone="red"
                  variant="outline"
                  disabled={connectionStatus === 'disconnecting'}
                  onPress={onDisconnect}
                >
                  {connectionStatus === 'disconnecting'
                    ? 'Disconnecting...'
                    : 'Disconnect'}
                </Button>
              ) : (
                <Button
                  disabled={
                    connectionStatus !== 'disconnected' &&
                    connectionStatus !== 'error'
                  }
                  onPress={() => onConnect(device)}
                >
                  Connect
                </Button>
              )}
            </View>
          </CardContent>
        </Card>
      ))}
    </View>
  );
}

const styles = StyleSheet.create({
  list: {
    width: '100%',
    gap: 12,
  },
  deviceCard: {
    width: '100%',
  },
  actions: {
    marginTop: 12,
  },
});
